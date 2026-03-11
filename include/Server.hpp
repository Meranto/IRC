/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:40:52 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:40:53 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <netinet/in.h>

class Client;
class Channel;

// Cœur du serveur IRC : gère le socket principal, la boucle poll(),
// l'enregistrement des clients/salons et le routage de toutes les commandes.
class Server {
private:
    int                             _socketFd;
    int                             _portNum;
    std::string                     _serverPass;
    std::vector<struct pollfd>      _fds;               // Tableau surveillé par poll()
    std::map<int, Client*>          _connectedClients;  // fd -> Client
    std::map<std::string, Channel*> _activeChannels;    // nom -> Channel
    bool                            _isRunning;

    // Initialisation du socket serveur
    void initializeServerSocket();
    void configureSocketOptions();
    void bindSocketToPort();
    void startListening();

    // Gestion des connexions et des données entrantes
    void onNewConnection();
    void onClientData(int fd);
    void disconnectClient(int fd);
    void extractAndProcessCommands(Client* client);
    void processIrcCommand(Client* client, const std::string& commandLine);

public:
    Server(int port, const std::string& password);
    ~Server();

    void start();
    void shutdown();

    const std::string& getServerPassword() const;
    Client* findClientByFd(int fd);
    Channel* findChannelByName(const std::string& name);

    void registerClient(int fd, Client* client);
    void unregisterClient(int fd);

    void registerChannel(const std::string& name, Channel* channel);
    void unregisterChannel(const std::string& name);

    void transmitToClient(int fd, const std::string& message);
    void broadcastToChannel(const std::string& channelName, const std::string& message, int senderFd = -1);

    void routeCommand(Client* client, const std::string& cmd, const std::vector<std::string>& params);

    // Utilitaires partagés entre les fichiers de commandes
    void sendWelcomeMessages(Client* client);
    bool isValidNickname(const std::string& nick);
    bool isValidChannelName(const std::string& channel);
    Client* findClientByNick(const std::string& nickname);
    bool isNicknameInUse(const std::string& nickname, Client* excludeClient = NULL);
    std::string formatNumericReply(int code, const std::string& clientNick, const std::string& message);
    void sendNumericReply(Client* client, int code, const std::string& message);
    void sendErrorMessage(Client* client, const std::string& command, const std::string& message);
    void broadcastToAll(const std::string& message);
    std::vector<std::string> getClientChannels(Client* client);
    void cleanupEmptyChannels();
    std::string trimString(const std::string& str);

    // Handlers des commandes IRC
    void handlePassCommand(Client* client, const std::vector<std::string>& params);
    void handleNickCommand(Client* client, const std::vector<std::string>& params);
    void handleUserCommand(Client* client, const std::vector<std::string>& params);
    void handleJoinCommand(Client* client, const std::vector<std::string>& params);
    void handlePartCommand(Client* client, const std::vector<std::string>& params);
    void handlePrivmsgCommand(Client* client, const std::vector<std::string>& params);
    void handleKickCommand(Client* client, const std::vector<std::string>& params);
    void handleInviteCommand(Client* client, const std::vector<std::string>& params);
    void handleTopicCommand(Client* client, const std::vector<std::string>& params);
    void handleModeCommand(Client* client, const std::vector<std::string>& params);
    void handlePingCommand(Client* client, const std::vector<std::string>& params);
    void handleQuitCommand(Client* client, const std::vector<std::string>& params);
};

#endif
