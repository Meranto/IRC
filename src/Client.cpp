/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:42:55 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:42:56 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int socketFd)
    : _socketFd(socketFd), _passVerified(false), _nickSet(false),
      _userSet(false), _fullyRegistered(false) {
}

Client::~Client() {
}

int Client::getSocketFd() const {
    return _socketFd;
}

const std::string& Client::getNick() const {
    return _nick;
}

const std::string& Client::getUser() const {
    return _user;
}

const std::string& Client::getRealName() const {
    return _realName;
}

const std::string& Client::getHostAddress() const {
    return _hostAddress;
}

const std::string& Client::getInputBuffer() const {
    return _inputBuffer;
}

bool Client::isPasswordVerified() const {
    return _passVerified;
}

bool Client::isNickSet() const {
    return _nickSet;
}

bool Client::isUserSet() const {
    return _userSet;
}

bool Client::isFullyRegistered() const {
    return _fullyRegistered;
}

// Met à jour le nick et déclenche la vérification de l'état d'enregistrement
void Client::setNick(const std::string& nickname) {
    _nick = nickname;
    _nickSet = true;
    updateRegistrationStatus();
}

// Met à jour le username et déclenche la vérification de l'état d'enregistrement
void Client::setUser(const std::string& username) {
    _user = username;
    _userSet = true;
    updateRegistrationStatus();
}

void Client::setRealName(const std::string& realname) {
    _realName = realname;
}

void Client::setHostAddress(const std::string& hostname) {
    _hostAddress = hostname;
}

// Marque le mot de passe comme vérifié et réévalue l'état d'enregistrement
void Client::markPasswordVerified() {
    _passVerified = true;
    updateRegistrationStatus();
}

void Client::appendToInputBuffer(const std::string& data) {
    _inputBuffer += data;
}

void Client::clearInputBuffer() {
    _inputBuffer.clear();
}

// Un client est considéré enregistré dès que PASS, NICK et USER ont tous été reçus
void Client::updateRegistrationStatus() {
    if (_passVerified && _nickSet && _userSet && !_fullyRegistered) {
        _fullyRegistered = true;
    }
}

// Construit le préfixe IRC au format nick!user@host utilisé dans les messages
std::string Client::buildPrefix() const {
    std::string prefix = _nick;
    if (!_user.empty())
        prefix += "!" + _user;
    if (!_hostAddress.empty())
        prefix += "@" + _hostAddress;
    return prefix;
}
