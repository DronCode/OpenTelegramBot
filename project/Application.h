#pragma once

#include <string>

namespace reactor {

    class Application
    {
        std::string m_telegramToken;
        std::string m_telegramProxy;

    public:
        int run();
    };

}