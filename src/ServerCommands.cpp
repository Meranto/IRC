/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerCommands.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:45:16 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:45:17 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>

// PING : mécanisme de keepalive — on répond simplement avec le token reçu
void Server::handlePingCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " PING :Not enough parameters");
        return;
    }
    std::string pongResponse = ":server PONG server :" + params[0];
    transmitToClient(client->getSocketFd(), pongResponse);
}

// QUIT : notifie le client de la fermeture puis déconnecte proprement
void Server::handleQuitCommand(Client* client, const std::vector<std::string>& params) {
    std::string quitMsg = params.empty() ? "Client quit" : params[0];
    std::string response = "ERROR :Closing connection: " + quitMsg;
    transmitToClient(client->getSocketFd(), response);
    std::cout << "[QUIT] " << client->getNick() << " a quitté : " << quitMsg << std::endl;
    disconnectClient(client->getSocketFd());
}

// JOIN : crée le salon s'il n'existe pas, vérifie les modes (+i, +k, +l)
// puis ajoute le client et envoie JOIN + topic + liste des membres
void Server::handleJoinCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " JOIN :Not enough parameters");
        return;
    }

    std::string channelName = params[0];
    std::string channelKey = params.size() > 1 ? params[1] : "";

    if (!isValidChannelName(channelName)) {
        transmitToClient(client->getSocketFd(),
            ":server 403 " + client->getNick() + " " + channelName + " :No such channel");
        return;
    }

    Channel* channel = findChannelByName(channelName);
    bool isNewChannel = (channel == NULL);

    if (isNewChannel) {
        channel = new Channel(channelName);
        registerChannel(channelName, channel);
        std::cout << "[CHANNEL] Nouveau salon créé : " << channelName << std::endl;
    }

    if (channel->isParticipant(client))
        return;

    if (channel->isInviteOnlyMode() && !channel->isOnInviteList(client->getNick())) {
        transmitToClient(client->getSocketFd(),
            ":server 473 " + client->getNick() + " " + channelName + " :Cannot join channel (+i)");
        return;
    }

    if (!channel->getAccessKey().empty() && channel->getAccessKey() != channelKey) {
        transmitToClient(client->getSocketFd(),
            ":server 475 " + client->getNick() + " " + channelName + " :Cannot join channel (+k)");
        return;
    }

    if (channel->getMaxUsers() > 0 && channel->getParticipantCount() >= channel->getMaxUsers()) {
        transmitToClient(client->getSocketFd(),
            ":server 471 " + client->getNick() + " " + channelName + " :Cannot join channel (+l)");
        return;
    }

    channel->addParticipant(client);

    // Le créateur du salon devient automatiquement opérateur
    if (isNewChannel)
        channel->promoteToOperator(client);

    channel->removeFromInviteList(client->getNick());

    // On notifie tous les membres du salon, y compris le nouvel arrivant
    std::string joinMsg = ":" + client->buildPrefix() + " JOIN " + channelName;
    const std::set<Client*>& participants = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = participants.begin();
         it != participants.end(); ++it) {
        transmitToClient((*it)->getSocketFd(), joinMsg);
    }

    if (!channel->getChannelTopic().empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 332 " + client->getNick() + " " + channelName + " :" + channel->getChannelTopic());
    } else {
        transmitToClient(client->getSocketFd(),
            ":server 331 " + client->getNick() + " " + channelName + " :No topic is set");
    }

    // Construction de la liste RPL_NAMREPLY avec le préfixe @ pour les opérateurs
    std::string namesList = "";
    const std::set<Client*>& memberList = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = memberList.begin();
         it != memberList.end(); ++it) {
        if (!namesList.empty())
            namesList += " ";
        if (channel->isChannelOperator(*it))
            namesList += "@";
        namesList += (*it)->getNick();
    }

    transmitToClient(client->getSocketFd(),
        ":server 353 " + client->getNick() + " = " + channelName + " :" + namesList);
    transmitToClient(client->getSocketFd(),
        ":server 366 " + client->getNick() + " " + channelName + " :End of NAMES list");

    std::cout << "[JOIN] " << client->getNick() << " a rejoint " << channelName << std::endl;
}

// PART : notifie les membres avant de retirer le client ; supprime le salon si vide
void Server::handlePartCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " PART :Not enough parameters");
        return;
    }

    std::string channelName = params[0];
    std::string partMsg = params.size() > 1 ? params[1] : "Leaving";

    Channel* channel = findChannelByName(channelName);
    if (!channel) {
        transmitToClient(client->getSocketFd(),
            ":server 403 " + client->getNick() + " " + channelName + " :No such channel");
        return;
    }

    if (!channel->isParticipant(client)) {
        transmitToClient(client->getSocketFd(),
            ":server 442 " + client->getNick() + " " + channelName + " :You're not on that channel");
        return;
    }

    std::string partNotification = ":" + client->buildPrefix() + " PART " + channelName + " :" + partMsg;
    const std::set<Client*>& participants = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = participants.begin();
         it != participants.end(); ++it) {
        transmitToClient((*it)->getSocketFd(), partNotification);
    }

    channel->removeParticipant(client);

    if (channel->getParticipantCount() == 0) {
        unregisterChannel(channelName);
        std::cout << "[CHANNEL] Salon vide supprimé : " << channelName << std::endl;
    }

    std::cout << "[PART] " << client->getNick() << " a quitté " << channelName << std::endl;
}

// PRIVMSG : envoi vers un salon (diffusion sauf expéditeur) ou vers un nick (privé)
void Server::handlePrivmsgCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 411 " + client->getNick() + " :No recipient given (PRIVMSG)");
        return;
    }
    if (params.size() < 2) {
        transmitToClient(client->getSocketFd(),
            ":server 412 " + client->getNick() + " :No text to send");
        return;
    }

    std::string target = params[0];
    std::string message = params[1];

    if (target[0] == '#' || target[0] == '&') {
        Channel* channel = findChannelByName(target);
        if (!channel) {
            transmitToClient(client->getSocketFd(),
                ":server 403 " + client->getNick() + " " + target + " :No such channel");
            return;
        }
        if (!channel->isParticipant(client)) {
            transmitToClient(client->getSocketFd(),
                ":server 404 " + client->getNick() + " " + target + " :Cannot send to channel");
            return;
        }

        // L'expéditeur ne reçoit pas son propre message
        std::string channelMsg = ":" + client->buildPrefix() + " PRIVMSG " + target + " :" + message;
        const std::set<Client*>& participants = channel->getParticipants();
        for (std::set<Client*>::const_iterator it = participants.begin();
             it != participants.end(); ++it) {
            if (*it == client)
                continue;
            transmitToClient((*it)->getSocketFd(), channelMsg);
        }
        std::cout << "[PRIVMSG] " << client->getNick() << " -> " << target << ": " << message << std::endl;
    }
    else {
        Client* targetClient = findClientByNick(target);
        if (!targetClient) {
            transmitToClient(client->getSocketFd(),
                ":server 401 " + client->getNick() + " " + target + " :No such nick");
            return;
        }
        std::string privateMsg = ":" + client->buildPrefix() + " PRIVMSG " + target + " :" + message;
        transmitToClient(targetClient->getSocketFd(), privateMsg);
        std::cout << "[PRIVMSG] " << client->getNick() << " -> " << target << " (privé)" << std::endl;
    }
}
