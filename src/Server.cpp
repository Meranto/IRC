/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:45:13 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:45:14 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdexcept>

Server::Server(int port, const std::string& password)
    : _socketFd(-1), _portNum(port), _serverPass(password), _isRunning(false) {
    initializeServerSocket();
}

// Le destructeur s'assure que tous les clients et salons alloués sont libérés
Server::~Server() {
    shutdown();
    if (_socketFd != -1)
        close(_socketFd);
    for (std::map<int, Client*>::iterator it = _connectedClients.begin();
         it != _connectedClients.end(); ++it)
        delete it->second;
    for (std::map<std::string, Channel*>::iterator it = _activeChannels.begin();
         it != _activeChannels.end(); ++it)
        delete it->second;
}

// Crée le socket, configure les options, lie le port et démarre l'écoute
// Ajoute ensuite le fd serveur dans le tableau poll pour surveiller les nouvelles connexions
void Server::initializeServerSocket() {
    std::cout << "[SERVER] Initialisation sur le port " << _portNum << std::endl;
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd < 0)
        throw std::runtime_error("Création du socket échouée");
    configureSocketOptions();
    bindSocketToPort();
    startListening();
    struct pollfd serverPollEntry;
    serverPollEntry.fd = _socketFd;
    serverPollEntry.events = POLLIN;
    serverPollEntry.revents = 0;
    _fds.push_back(serverPollEntry);
    std::cout << "[SERVER] Prêt et en écoute !" << std::endl;
}

// SO_REUSEADDR évite l'erreur "Address already in use" après un redémarrage rapide
// O_NONBLOCK est indispensable pour que poll() fonctionne sans bloquer
void Server::configureSocketOptions() {
    int reuseAddr = 1;
    if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr)) < 0) {
        close(_socketFd);
        throw std::runtime_error("Configuration des options du socket échouée");
    }
    int flags = fcntl(_socketFd, F_GETFL, 0);
    if (flags < 0 || fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(_socketFd);
        throw std::runtime_error("Passage en mode non-bloquant échoué");
    }
}

void Server::bindSocketToPort() {
    struct sockaddr_in serverAddress;
    std::memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(_portNum);
    if (bind(_socketFd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        close(_socketFd);
        throw std::runtime_error("Liaison au port échouée — port déjà utilisé ?");
    }
}

void Server::startListening() {
    const int maxPendingConnections = 10;
    if (listen(_socketFd, maxPendingConnections) < 0) {
        close(_socketFd);
        throw std::runtime_error("listen() a échoué");
    }
}

// Boucle principale basée sur poll() : bloque jusqu'à une activité sur un fd
// On parcourt tous les fds actifs : le fd serveur signale une nouvelle connexion,
// les fds clients signalent des données à lire ou une déconnexion
void Server::start() {
    _isRunning = true;
    std::cout << "[SERVER] Boucle d'événements démarrée. En attente de connexions..." << std::endl;
    while (_isRunning) {
        int eventCount = poll(&_fds[0], _fds.size(), -1);
        if (eventCount < 0) {
            if (_isRunning)
                std::cerr << "[ERREUR] poll() a échoué" << std::endl;
            break;
        }
        for (size_t idx = 0; idx < _fds.size() && eventCount > 0; idx++) {
            if (_fds[idx].revents == 0)
                continue;
            eventCount--;
            int currentFd = _fds[idx].fd;
            if (currentFd == _socketFd) {
                onNewConnection();
                continue;
            }
            if (_fds[idx].revents & POLLIN) {
                onClientData(currentFd);
            } else if (_fds[idx].revents & (POLLHUP | POLLERR | POLLNVAL)) {
                std::cout << "[INFO] Connexion terminée (fd:" << currentFd << ")" << std::endl;
                disconnectClient(currentFd);
                idx--;
            }
        }
    }
    std::cout << "[SERVER] Boucle d'événements terminée" << std::endl;
}

void Server::shutdown() {
    _isRunning = false;
}

// Accepte la connexion entrante, passe le fd client en non-bloquant,
// résout l'adresse IP et enregistre le client dans la map + le tableau poll
void Server::onNewConnection() {
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);
    int clientFd = accept(_socketFd, (struct sockaddr*)&clientAddress, &clientAddrLen);
    if (clientFd < 0) {
        std::cerr << "[ERREUR] Impossible d'accepter la connexion cliente" << std::endl;
        return;
    }
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags < 0 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "[ERREUR] Passage en non-bloquant du socket client échoué" << std::endl;
        close(clientFd);
        return;
    }
    char hostBuffer[NI_MAXHOST];
    int result = getnameinfo((struct sockaddr*)&clientAddress, clientAddrLen,
                            hostBuffer, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
    std::string clientHost = (result == 0) ? hostBuffer : "unknown";
    Client* newClient = new Client(clientFd);
    newClient->setHostAddress(clientHost);
    registerClient(clientFd, newClient);
    struct pollfd clientPollEntry;
    clientPollEntry.fd = clientFd;
    clientPollEntry.events = POLLIN;
    clientPollEntry.revents = 0;
    _fds.push_back(clientPollEntry);
    std::cout << "[CONNECT] Nouveau client (fd:" << clientFd << ", host:" << clientHost << ")" << std::endl;
}

// Lit les données disponibles dans le socket et les accumule dans le buffer du client
// Si recv retourne 0 ou une erreur, on déconnecte proprement
void Server::onClientData(int fd) {
    char receiveBuffer[1024];
    std::memset(receiveBuffer, 0, sizeof(receiveBuffer));
    ssize_t bytesRead = recv(fd, receiveBuffer, sizeof(receiveBuffer) - 1, 0);
    if (bytesRead <= 0) {
        if (bytesRead == 0)
            std::cout << "[DISCONNECT] Client a fermé la connexion (fd:" << fd << ")" << std::endl;
        else
            std::cerr << "[ERREUR] recv() a échoué pour le client (fd:" << fd << ")" << std::endl;
        disconnectClient(fd);
        return;
    }
    receiveBuffer[bytesRead] = '\0';
    Client* client = findClientByFd(fd);
    if (!client) {
        std::cerr << "[ERREUR] Client introuvable dans le registre (fd:" << fd << ")" << std::endl;
        return;
    }
    client->appendToInputBuffer(std::string(receiveBuffer, bytesRead));
    extractAndProcessCommands(client);
}

// Extrait les commandes complètes du buffer (délimitées par \n) et les traite
// Gère à la fois les fins de ligne \r\n et \n seul
void Server::extractAndProcessCommands(Client* client) {
    std::string& buffer = const_cast<std::string&>(client->getInputBuffer());
    size_t pos;
    while ((pos = buffer.find('\n')) != std::string::npos) {
        std::string commandLine = buffer.substr(0, pos);
        buffer.erase(0, pos + 1);
        if (!commandLine.empty() && commandLine[commandLine.length() - 1] == '\r')
            commandLine.erase(commandLine.length() - 1);
        if (commandLine.empty())
            continue;
        std::cout << "[RECV fd:" << client->getSocketFd() << "] " << commandLine << std::endl;
        processIrcCommand(client, commandLine);
    }
}

// Notifie les salons communs du départ du client, retire le fd de poll, ferme la socket
void Server::disconnectClient(int fd) {
    Client* client = findClientByFd(fd);
    if (client) {
        std::string quitMessage = ":" + client->buildPrefix() + " QUIT :Connection closed\r\n";
        for (std::map<std::string, Channel*>::iterator it = _activeChannels.begin();
             it != _activeChannels.end(); ++it) {
            if (it->second->isParticipant(client)) {
                const std::set<Client*>& participants = it->second->getParticipants();
                for (std::set<Client*>::const_iterator pit = participants.begin();
                     pit != participants.end(); ++pit) {
                    if (*pit != client)
                        transmitToClient((*pit)->getSocketFd(), quitMessage);
                }
                it->second->removeParticipant(client);
            }
        }
    }
    for (std::vector<struct pollfd>::iterator it = _fds.begin(); it != _fds.end(); ++it) {
        if (it->fd == fd) {
            _fds.erase(it);
            break;
        }
    }
    close(fd);
    unregisterClient(fd);
    std::cout << "[DISCONNECT] Client supprimé (fd:" << fd << ")" << std::endl;
}

const std::string& Server::getServerPassword() const {
    return _serverPass;
}

Client* Server::findClientByFd(int fd) {
    std::map<int, Client*>::iterator it = _connectedClients.find(fd);
    if (it != _connectedClients.end())
        return it->second;
    return NULL;
}

Channel* Server::findChannelByName(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _activeChannels.find(name);
    if (it != _activeChannels.end())
        return it->second;
    return NULL;
}

void Server::registerClient(int fd, Client* client) {
    _connectedClients[fd] = client;
}

void Server::unregisterClient(int fd) {
    std::map<int, Client*>::iterator it = _connectedClients.find(fd);
    if (it != _connectedClients.end()) {
        delete it->second;
        _connectedClients.erase(it);
    }
}

void Server::registerChannel(const std::string& name, Channel* channel) {
    _activeChannels[name] = channel;
}

void Server::unregisterChannel(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _activeChannels.find(name);
    if (it != _activeChannels.end()) {
        delete it->second;
        _activeChannels.erase(it);
    }
}

// Ajoute \r\n si absent avant d'envoyer — le protocole IRC l'exige
void Server::transmitToClient(int fd, const std::string& message) {
    if (message.empty())
        return;
    std::string formattedMsg = message;
    if (formattedMsg.size() < 2 ||
        formattedMsg.substr(formattedMsg.size() - 2) != "\r\n") {
        formattedMsg += "\r\n";
    }
    ssize_t bytesSent = send(fd, formattedMsg.c_str(), formattedMsg.length(), 0);
    if (bytesSent < 0)
        std::cerr << "[ERREUR] Envoi échoué vers le client (fd:" << fd << ")" << std::endl;
}

// Découpe la ligne en tokens en respectant le paramètre trailing (précédé de ':')
// qui peut contenir des espaces — convertit la commande en majuscules pour être insensible à la casse
void Server::processIrcCommand(Client* client, const std::string& commandLine) {
    if (commandLine.empty())
        return;
    std::vector<std::string> tokens;
    std::string currentToken;
    bool inTrailingParam = false;
    for (size_t i = 0; i < commandLine.length(); i++) {
        char c = commandLine[i];
        if (c == ':' && !inTrailingParam && currentToken.empty()) {
            inTrailingParam = true;
            tokens.push_back(commandLine.substr(i + 1));
            break;
        } else if (c == ' ' && !inTrailingParam) {
            if (!currentToken.empty()) {
                tokens.push_back(currentToken);
                currentToken.clear();
            }
        } else {
            currentToken += c;
        }
    }
    if (!currentToken.empty() && !inTrailingParam)
        tokens.push_back(currentToken);
    if (tokens.empty())
        return;
    std::string command = tokens[0];
    for (size_t i = 0; i < command.length(); i++)
        command[i] = std::toupper(command[i]);
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());
    routeCommand(client, command, params);
}

// PASS, NICK, USER, PING et QUIT sont accessibles avant l'enregistrement complet
// Toute autre commande requiert que le client soit pleinement enregistré
void Server::routeCommand(Client* client, const std::string& cmd,
                         const std::vector<std::string>& params) {
    if (cmd == "PASS")
        handlePassCommand(client, params);
    else if (cmd == "NICK")
        handleNickCommand(client, params);
    else if (cmd == "USER")
        handleUserCommand(client, params);
    else if (cmd == "PING")
        handlePingCommand(client, params);
    else if (cmd == "QUIT")
        handleQuitCommand(client, params);
    else if (!client->isFullyRegistered()) {
        transmitToClient(client->getSocketFd(),
            ":server 451 * :You have not registered");
    }
    else if (cmd == "JOIN")
        handleJoinCommand(client, params);
    else if (cmd == "PART")
        handlePartCommand(client, params);
    else if (cmd == "PRIVMSG")
        handlePrivmsgCommand(client, params);
    else if (cmd == "KICK")
        handleKickCommand(client, params);
    else if (cmd == "INVITE")
        handleInviteCommand(client, params);
    else if (cmd == "TOPIC")
        handleTopicCommand(client, params);
    else if (cmd == "MODE")
        handleModeCommand(client, params);
    else {
        std::string response = ":server 421 " + client->getNick() + " " + cmd + " :Unknown command";
        transmitToClient(client->getSocketFd(), response);
    }
}

// Diffuse un message à tous les membres du salon, en excluant l'expéditeur si senderFd != -1
void Server::broadcastToChannel(const std::string& channelName, const std::string& message, int senderFd) {
    Channel* channel = findChannelByName(channelName);
    if (!channel)
        return;
    const std::set<Client*>& participants = channel->getParticipants();
    for (std::set<Client*>::const_iterator it = participants.begin();
         it != participants.end(); ++it) {
        if (senderFd != -1 && (*it)->getSocketFd() == senderFd)
            continue;
        transmitToClient((*it)->getSocketFd(), message);
    }
}

// PASS : vérifie le mot de passe avant tout enregistrement
// En cas d'erreur, la connexion est immédiatement fermée
void Server::handlePassCommand(Client* client, const std::vector<std::string>& params) {
    if (client->isFullyRegistered()) {
        transmitToClient(client->getSocketFd(),
            ":server 462 " + client->getNick() + " :You may not reregister");
        return;
    }
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 461 * PASS :Not enough parameters");
        return;
    }
    if (params[0] != _serverPass) {
        transmitToClient(client->getSocketFd(),
            ":server 464 * :Password incorrect");
        disconnectClient(client->getSocketFd());
        return;
    }
    client->markPasswordVerified();
    std::cout << "[AUTH] Mot de passe vérifié pour fd:" << client->getSocketFd() << std::endl;
}

// NICK : valide le format, vérifie l'unicité et notifie les salons communs en cas de changement
void Server::handleNickCommand(Client* client, const std::vector<std::string>& params) {
    if (params.empty()) {
        transmitToClient(client->getSocketFd(),
            ":server 431 * :No nickname given");
        return;
    }
    std::string requestedNick = params[0];
    if (!isValidNickname(requestedNick)) {
        transmitToClient(client->getSocketFd(),
            ":server 432 * " + requestedNick + " :Erroneous nickname");
        return;
    }
    if (isNicknameInUse(requestedNick, client)) {
        transmitToClient(client->getSocketFd(),
            ":server 433 * " + requestedNick + " :Nickname is already in use");
        return;
    }
    std::string oldNick = client->getNick();
    bool hadNick = !oldNick.empty();
    client->setNick(requestedNick);
    if (hadNick && client->isFullyRegistered()) {
        std::string nickChange = ":" + oldNick + " NICK :" + requestedNick;
        transmitToClient(client->getSocketFd(), nickChange);
        // Propage le changement de nick à tous les salons où le client est présent
        std::vector<std::string> userChannels = getClientChannels(client);
        for (size_t i = 0; i < userChannels.size(); i++) {
            Channel* chan = findChannelByName(userChannels[i]);
            if (chan) {
                const std::set<Client*>& members = chan->getParticipants();
                for (std::set<Client*>::const_iterator it = members.begin();
                     it != members.end(); ++it) {
                    if (*it != client)
                        transmitToClient((*it)->getSocketFd(), nickChange);
                }
            }
        }
    }
    if (client->isFullyRegistered() && !hadNick)
        sendWelcomeMessages(client);
    std::cout << "[NICK] Client fd:" << client->getSocketFd() << " s'appelle maintenant : " << requestedNick << std::endl;
}

// USER : hostname et servername sont ignorés (connexion directe sans relais)
// L'enregistrement complet est déclenché si PASS et NICK ont déjà été reçus
void Server::handleUserCommand(Client* client, const std::vector<std::string>& params) {
    if (client->isUserSet()) {
        transmitToClient(client->getSocketFd(),
            ":server 462 " + client->getNick() + " :You may not reregister");
        return;
    }
    if (params.size() < 4) {
        transmitToClient(client->getSocketFd(),
            ":server 461 * USER :Not enough parameters");
        return;
    }
    std::string username = params[0];
    std::string realname = params[3];
    client->setUser(username);
    client->setRealName(realname);
    if (client->isFullyRegistered())
        sendWelcomeMessages(client);
    std::cout << "[USER] Client fd:" << client->getSocketFd()
              << " user:" << username << " realname:" << realname << std::endl;
}

// Envoie la séquence de bienvenue IRC (001-004) dès que l'enregistrement est complet
void Server::sendWelcomeMessages(Client* client) {
    std::string nick = client->getNick();
    transmitToClient(client->getSocketFd(),
        ":server 001 " + nick + " :Welcome to the IRC Network " + client->buildPrefix());
    transmitToClient(client->getSocketFd(),
        ":server 002 " + nick + " :Your host is server, running version 1.0");
    transmitToClient(client->getSocketFd(),
        ":server 003 " + nick + " :This server was created today");
    transmitToClient(client->getSocketFd(),
        ":server 004 " + nick + " server 1.0 o itkol");
    std::cout << "[ENREGISTRÉ] Client " << nick << " (fd:" << client->getSocketFd() << ") complètement enregistré" << std::endl;
}
