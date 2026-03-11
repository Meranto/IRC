/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:40:50 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:40:51 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>

// Représente un client connecté au serveur IRC.
// L'enregistrement est considéré complet quand PASS, NICK et USER ont été reçus.
class Client {
private:
    int         _socketFd;
    std::string _nick;
    std::string _user;
    std::string _realName;
    std::string _hostAddress;
    std::string _inputBuffer;   // Accumule les données reçues entre deux appels à recv()

    bool        _passVerified;
    bool        _nickSet;
    bool        _userSet;
    bool        _fullyRegistered;

public:
    Client(int socketFd);
    ~Client();

    int getSocketFd() const;

    const std::string& getNick() const;
    const std::string& getUser() const;
    const std::string& getRealName() const;
    const std::string& getHostAddress() const;
    const std::string& getInputBuffer() const;

    bool isPasswordVerified() const;
    bool isNickSet() const;
    bool isUserSet() const;
    bool isFullyRegistered() const;

    void setNick(const std::string& nickname);
    void setUser(const std::string& username);
    void setRealName(const std::string& realname);
    void setHostAddress(const std::string& hostname);

    void markPasswordVerified();

    void appendToInputBuffer(const std::string& data);
    void clearInputBuffer();

    // Appelé par les setters pour vérifier si les 3 conditions sont réunies
    void updateRegistrationStatus();

    // Retourne nick!user@host pour les messages IRC sortants
    std::string buildPrefix() const;
};

#endif
