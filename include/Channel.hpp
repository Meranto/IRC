/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:40:45 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:45:03 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <set>

class Client;

// Représente un salon IRC avec ses membres, opérateurs, modes et liste d'invitations.
// La gestion des messages passe par Server::broadcastToChannel, pas par cette classe.
class Channel {
private:
    std::string             _channelName;
    std::string             _channelTopic;
    std::string             _accessKey;         // Clé d'accès (mode +k)
    std::set<Client*>       _participants;
    std::set<Client*>       _channelOps;
    std::set<std::string>   _invitedNicks;
    size_t                  _maxUsers;          // Limite d'utilisateurs (mode +l), 0 = illimité

    bool                    _inviteOnlyMode;    // Mode +i
    bool                    _topicOpOnly;       // Mode +t

public:
    Channel(const std::string& name);
    ~Channel();

    const std::string& getChannelName() const;
    const std::string& getChannelTopic() const;
    const std::string& getAccessKey() const;
    size_t getMaxUsers() const;

    bool isInviteOnlyMode() const;
    bool isTopicOpOnly() const;
    const std::set<Client*>& getParticipants() const;

    void setChannelTopic(const std::string& topic);
    void setAccessKey(const std::string& key);
    void setMaxUsers(size_t limit);

    void setInviteOnlyMode(bool enabled);
    void setTopicOpOnly(bool restricted);

    void addParticipant(Client* client);
    void removeParticipant(Client* client);
    bool isParticipant(Client* client) const;

    void promoteToOperator(Client* client);
    void demoteFromOperator(Client* client);
    bool isChannelOperator(Client* client) const;

    void addToInviteList(const std::string& nickname);
    void removeFromInviteList(const std::string& nickname);
    bool isOnInviteList(const std::string& nickname) const;

    // Stub — la diffusion réelle est gérée par le Server
    void distributeMessage(const std::string& message, Client* sender = NULL);

    size_t getParticipantCount() const;
};

#endif
