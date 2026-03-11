/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerOperatorCommands.cpp                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:45:23 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:45:24 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <cstdlib>

// KICK : réservé aux opérateurs — notifie tout le salon avant de retirer la cible
void Server::handleKickCommand(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " KICK :Not enough parameters");
        return;
    }

    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = params.size() > 2 ? params[2] : client->getNick();

    Channel* channel = findChannelByName(channelName);
    if (!channel) {
        transmitToClient(client->getSocketFd(),
            ":server 403 " + client->getNick() + " " + channelName + " :No such channel");
        return;
    }

    if (!channel->isChannelOperator(client)) {
        transmitToClient(client->getSocketFd(),
            ":server 482 " + client->getNick() + " " + channelName + " :You're not channel operator");
        return;
    }

    Client* targetClient = findClientByNick(targetNick);
    if (!targetClient || !channel->isParticipant(targetClient)) {
        transmitToClient(client->getSocketFd(),
            ":server 441 " + client->getNick() + " " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string kickMsg = ":" + client->buildPrefix() + " KICK " + channelName + " " + targetNick + " :" + reason;
    const std::set<Client*>& participants = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = participants.begin();
         it != participants.end(); ++it) {
        transmitToClient((*it)->getSocketFd(), kickMsg);
    }

    channel->removeParticipant(targetClient);
    std::cout << "[KICK] " << client->getNick() << " a expulsé " << targetNick << " de " << channelName << std::endl;
}

// INVITE : ajoute le nick cible à la liste d'invitations et notifie les deux parties
// Sur un salon +i, seul un opérateur peut inviter
void Server::handleInviteCommand(Client* client, const std::vector<std::string>& params) {
    if (params.size() < 2) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " INVITE :Not enough parameters");
        return;
    }

    std::string targetNick = params[0];
    std::string channelName = params[1];

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

    if (channel->isInviteOnlyMode() && !channel->isChannelOperator(client)) {
        transmitToClient(client->getSocketFd(),
            ":server 482 " + client->getNick() + " " + channelName + " :You're not channel operator");
        return;
    }

    Client* targetClient = findClientByNick(targetNick);
    if (!targetClient) {
        transmitToClient(client->getSocketFd(),
            ":server 401 " + client->getNick() + " " + targetNick + " :No such nick");
        return;
    }

    if (channel->isParticipant(targetClient)) {
        transmitToClient(client->getSocketFd(),
            ":server 443 " + client->getNick() + " " + targetNick + " " + channelName + " :is already on channel");
        return;
    }

    channel->addToInviteList(targetNick);

    transmitToClient(client->getSocketFd(),
        ":server 341 " + client->getNick() + " " + targetNick + " " + channelName);

    std::string inviteMsg = ":" + client->buildPrefix() + " INVITE " + targetNick + " " + channelName;
    transmitToClient(targetClient->getSocketFd(), inviteMsg);

    std::cout << "[INVITE] " << client->getNick() << " a invité " << targetNick << " dans " << channelName << std::endl;
}

// TOPIC sans argument affiche le topic actuel ; avec argument, le modifie
// La modification est restreinte aux opérateurs si le mode +t est actif
void Server::handleTopicCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " TOPIC :Not enough parameters");
        return;
    }

    std::string channelName = params[0];

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

    if (params.size() == 1) {
        if (channel->getChannelTopic().empty()) {
            transmitToClient(client->getSocketFd(),
                ":server 331 " + client->getNick() + " " + channelName + " :No topic is set");
        } else {
            transmitToClient(client->getSocketFd(),
                ":server 332 " + client->getNick() + " " + channelName + " :" + channel->getChannelTopic());
        }
        return;
    }

    if (channel->isTopicOpOnly() && !channel->isChannelOperator(client)) {
        transmitToClient(client->getSocketFd(),
            ":server 482 " + client->getNick() + " " + channelName + " :You're not channel operator");
        return;
    }

    std::string newTopic = params[1];
    channel->setChannelTopic(newTopic);

    std::string topicMsg = ":" + client->buildPrefix() + " TOPIC " + channelName + " :" + newTopic;
    const std::set<Client*>& participants = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = participants.begin();
         it != participants.end(); ++it) {
        transmitToClient((*it)->getSocketFd(), topicMsg);
    }

    std::cout << "[TOPIC] " << client->getNick() << " a changé le topic de " << channelName << std::endl;
}

// MODE : traite la chaîne de modes caractère par caractère
// Modes supportés : i (invite-only), t (topic op-only), k (clé), l (limite), o (op)
void Server::handleModeCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 461 " + client->getNick() + " MODE :Not enough parameters");
        return;
    }

    std::string channelName = params[0];

    Channel* channel = findChannelByName(channelName);
    if (!channel) {
        transmitToClient(client->getSocketFd(),
            ":server 403 " + client->getNick() + " " + channelName + " :No such channel");
        return;
    }

    if (!channel->isChannelOperator(client)) {
        transmitToClient(client->getSocketFd(),
            ":server 482 " + client->getNick() + " " + channelName + " :You're not channel operator");
        return;
    }

    // Sans argument de mode, on affiche les modes actuellement actifs
    if (params.size() == 1) {
        std::string modes = "+";
        if (channel->isInviteOnlyMode()) modes += "i";
        if (channel->isTopicOpOnly()) modes += "t";
        if (!channel->getAccessKey().empty()) modes += "k";
        if (channel->getMaxUsers() > 0) modes += "l";
        transmitToClient(client->getSocketFd(),
            ":server 324 " + client->getNick() + " " + channelName + " " + modes);
        return;
    }

    std::string modeString = params[1];
    bool adding = true;
    size_t paramIndex = 2;

    for (size_t i = 0; i < modeString.length(); i++) {
        char mode = modeString[i];
        if (mode == '+')
            adding = true;
        else if (mode == '-')
            adding = false;
        else if (mode == 'i')
            channel->setInviteOnlyMode(adding);
        else if (mode == 't')
            channel->setTopicOpOnly(adding);
        else if (mode == 'k') {
            if (adding && paramIndex < params.size())
                channel->setAccessKey(params[paramIndex++]);
            else if (!adding)
                channel->setAccessKey("");
        }
        else if (mode == 'l') {
            if (adding && paramIndex < params.size()) {
                int limit = atoi(params[paramIndex++].c_str());
                if (limit > 0)
                    channel->setMaxUsers(limit);
            } else if (!adding) {
                channel->setMaxUsers(0);
            }
        }
        else if (mode == 'o') {
            if (paramIndex < params.size()) {
                std::string targetNick = params[paramIndex++];
                Client* targetClient = findClientByNick(targetNick);
                if (targetClient && channel->isParticipant(targetClient)) {
                    if (adding)
                        channel->promoteToOperator(targetClient);
                    else
                        channel->demoteFromOperator(targetClient);
                }
            }
        }
    }

    // Notifie tous les membres du changement de mode
    std::string modeMsg = ":" + client->buildPrefix() + " MODE " + channelName + " " + params[1];
    for (size_t i = 2; i < params.size(); i++)
        modeMsg += " " + params[i];

    const std::set<Client*>& participants = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = participants.begin();
         it != participants.end(); ++it) {
        transmitToClient((*it)->getSocketFd(), modeMsg);
    }

    std::cout << "[MODE] " << client->getNick() << " a modifié les modes de " << channelName << std::endl;
}
