/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anmerten <anmerten@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/11 16:43:32 by anmerten          #+#    #+#             */
/*   Updated: 2026/03/11 16:43:33 by anmerten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

// Pointeur global pour permettre l'arrêt propre depuis le handler de signal
Server* g_serverInstance = NULL;

// Intercepte SIGINT/SIGQUIT pour déclencher l'arrêt du serveur proprement
void handleInterruptSignal(int sig) {
    (void)sig;
    std::cout << "\n[SHUTDOWN] Signal d'interruption reçu" << std::endl;
    if (g_serverInstance) {
        g_serverInstance->shutdown();
    }
}

// Vérifie que le port est bien un entier dans la plage 1-65535
bool validatePortNumber(const std::string& portStr) {
    for (size_t i = 0; i < portStr.length(); i++) {
        if (!isdigit(portStr[i]))
            return false;
    }
    int portValue = atoi(portStr.c_str());
    return (portValue >= 1 && portValue <= 65535);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        std::cerr << "  port: 1-65535" << std::endl;
        std::cerr << "  password: mot de passe de connexion au serveur" << std::endl;
        return 1;
    }

    std::string portArg = argv[1];
    std::string passwordArg = argv[2];

    if (!validatePortNumber(portArg)) {
        std::cerr << "[ERREUR] Port invalide. Doit être numérique (1-65535)" << std::endl;
        return 1;
    }

    if (passwordArg.empty()) {
        std::cerr << "[ERREUR] Le mot de passe ne peut pas être vide" << std::endl;
        return 1;
    }

    int portNumber = atoi(portArg.c_str());

    try {
        Server ircServer(portNumber, passwordArg);
        g_serverInstance = &ircServer;

        // Enregistrement des signaux pour un arrêt propre
        signal(SIGINT, handleInterruptSignal);   // Ctrl+C
        signal(SIGQUIT, handleInterruptSignal);  // Ctrl+backslash

        std::cout << "========================================" << std::endl;
        std::cout << "  IRC Server v1.0" << std::endl;
        std::cout << "  Port: " << portNumber << std::endl;
        std::cout << "========================================" << std::endl;

        ircServer.start();

        std::cout << "[INFO] Arrêt du serveur terminé" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
