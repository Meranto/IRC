/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:45:27 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:45:28 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <sstream>

// Le premier caractère doit être une lettre ; les suivants peuvent être
// alphanumériques, underscore, tiret ou crochets (RFC 1459)
bool Server::isValidNickname(const std::string& nick) {
    if (nick.empty() || nick.length() > 9)
        return false;
    if (!std::isalpha(nick[0]))
        return false;
    for (size_t i = 1; i < nick.length(); i++) {
        char c = nick[i];
        if (!std::isalnum(c) && c != '_' && c != '-' && c != '[' && c != ']')
            return false;
    }
    return true;
}

// Le nom doit commencer par # ou & et ne pas contenir d'espaces, virgules
// ni caractères de contrôle (longueur max 50)
bool Server::isValidChannelName(const std::string& channel) {
    if (channel.empty() || channel.length() > 50)
        return false;
    if (channel[0] != '#' && channel[0] != '&')
        return false;
    for (size_t i = 1; i < channel.length(); i++) {
        char c = channel[i];
        if (c == ' ' || c == ',' || c == '\a' || c < 33 || c > 126)
            return false;
    }
    return true;
}

// Parcourt tous les clients connectés et retourne celui dont le nick correspond
Client* Server::findClientByNick(const std::string& nickname) {
    for (std::map<int, Client*>::iterator it = _connectedClients.begin();
         it != _connectedClients.end(); ++it) {
        if (it->second->getNick() == nickname)
            return it->second;
    }
    return NULL;
}

// Permet d'exclure un client de la vérification (utile lors d'un changement de nick)
bool Server::isNicknameInUse(const std::string& nickname, Client* excludeClient) {
    for (std::map<int, Client*>::iterator it = _connectedClients.begin();
         it != _connectedClients.end(); ++it) {
        if (it->second != excludeClient && it->second->getNick() == nickname)
            return true;
    }
    return false;
}

// Formate un code numérique IRC sur 3 chiffres avec préfixe ":server"
std::string Server::formatNumericReply(int code, const std::string& clientNick,
                                       const std::string& message) {
    std::ostringstream oss;
    oss << ":server ";
    if (code < 100)
        oss << "0";
    if (code < 10)
        oss << "0";
    oss << code;
    oss << " " << (clientNick.empty() ? "*" : clientNick) << " " << message;
    return oss.str();
}

void Server::sendNumericReply(Client* client, int code, const std::string& message) {
    std::string reply = formatNumericReply(code, client->getNick(), message);
    transmitToClient(client->getSocketFd(), reply);
}

void Server::sendErrorMessage(Client* client, const std::string& command,
                               const std::string& message) {
    std::string error = ":server " + command + " :" + message;
    transmitToClient(client->getSocketFd(), error);
}

// Diffuse un message à tous les clients complètement enregistrés (annonces globales)
void Server::broadcastToAll(const std::string& message) {
    for (std::map<int, Client*>::iterator it = _connectedClients.begin();
         it != _connectedClients.end(); ++it) {
        if (it->second->isFullyRegistered())
            transmitToClient(it->second->getSocketFd(), message);
    }
}

// Retourne la liste des noms de salons dont le client est membre
std::vector<std::string> Server::getClientChannels(Client* client) {
    std::vector<std::string> channels;
    for (std::map<std::string, Channel*>::iterator it = _activeChannels.begin();
         it != _activeChannels.end(); ++it) {
        if (it->second->isParticipant(client))
            channels.push_back(it->first);
    }
    return channels;
}

// Supprime les salons vides de la map pour éviter les fuites mémoire
void Server::cleanupEmptyChannels() {
    std::vector<std::string> toRemove;
    for (std::map<std::string, Channel*>::iterator it = _activeChannels.begin();
         it != _activeChannels.end(); ++it) {
        if (it->second->getParticipantCount() == 0)
            toRemove.push_back(it->first);
    }
    for (size_t i = 0; i < toRemove.size(); i++) {
        unregisterChannel(toRemove[i]);
        std::cout << "[CLEANUP] Salon vide supprimé : " << toRemove[i] << std::endl;
    }
}

// Supprime les espaces en début et fin de chaîne
std::string Server::trimString(const std::string& str) {
    size_t start = 0;
    size_t end = str.length();
    while (start < end && std::isspace(str[start]))
        start++;
    while (end > start && std::isspace(str[end - 1]))
        end--;
    return str.substr(start, end - start);
}
