/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:44:38 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:44:39 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"
#include "Client.hpp"

// Le mode +t (topic réservé aux ops) est activé par défaut, conformément au comportement IRC standard
Channel::Channel(const std::string& name)
    : _channelName(name), _maxUsers(0), _inviteOnlyMode(false), _topicOpOnly(true) {
}

Channel::~Channel() {
}

const std::string& Channel::getChannelName() const {
    return _channelName;
}

const std::string& Channel::getChannelTopic() const {
    return _channelTopic;
}

const std::string& Channel::getAccessKey() const {
    return _accessKey;
}

size_t Channel::getMaxUsers() const {
    return _maxUsers;
}

bool Channel::isInviteOnlyMode() const {
    return _inviteOnlyMode;
}

bool Channel::isTopicOpOnly() const {
    return _topicOpOnly;
}

const std::set<Client*>& Channel::getParticipants() const {
    return _participants;
}

void Channel::setChannelTopic(const std::string& topic) {
    _channelTopic = topic;
}

void Channel::setAccessKey(const std::string& key) {
    _accessKey = key;
}

void Channel::setMaxUsers(size_t limit) {
    _maxUsers = limit;
}

void Channel::setInviteOnlyMode(bool enabled) {
    _inviteOnlyMode = enabled;
}

void Channel::setTopicOpOnly(bool restricted) {
    _topicOpOnly = restricted;
}

void Channel::addParticipant(Client* client) {
    _participants.insert(client);
}

// Retire aussi le client des opérateurs si nécessaire
void Channel::removeParticipant(Client* client) {
    _participants.erase(client);
    _channelOps.erase(client);
}

bool Channel::isParticipant(Client* client) const {
    return _participants.find(client) != _participants.end();
}

void Channel::promoteToOperator(Client* client) {
    _channelOps.insert(client);
}

void Channel::demoteFromOperator(Client* client) {
    _channelOps.erase(client);
}

bool Channel::isChannelOperator(Client* client) const {
    return _channelOps.find(client) != _channelOps.end();
}

void Channel::addToInviteList(const std::string& nickname) {
    _invitedNicks.insert(nickname);
}

void Channel::removeFromInviteList(const std::string& nickname) {
    _invitedNicks.erase(nickname);
}

bool Channel::isOnInviteList(const std::string& nickname) const {
    return _invitedNicks.find(nickname) != _invitedNicks.end();
}

// La diffusion réelle passe par Server::broadcastToChannel — cette méthode
// existe uniquement pour satisfaire l'interface sans casser la compilation
void Channel::distributeMessage(const std::string& message, Client* sender) {
    (void)message;
    (void)sender;
}

size_t Channel::getParticipantCount() const {
    return _participants.size();
}
