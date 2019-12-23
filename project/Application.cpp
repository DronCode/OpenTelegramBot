#include <Application.h>

#include <cstring>
#include <cstdint>

#include <list>
#include <queue>
#include <thread>
#include <string>
#include <memory>
#include <optional>
#include <stdexcept>
#include <functional>
#include <unordered_map>

#include <spdlog/spdlog.h>
#include <curl/curl.h>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace reactor::telegram::exceptions {

    class BadAuthorization : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Your access token not acceptable by Telegram server.";
        }
    };

    class UnknownError : public std::exception {
        std::string m_errorMessage;
    public:
        explicit UnknownError(int32_t errorCode)
        {
            m_errorMessage = fmt::format("Unknown API error. Error code : {}", errorCode);
        }

        const char* what() const noexcept override {
            return m_errorMessage.c_str();
        }
    };

    class BotNotFound : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Your bot not found. Possible your token isn't correct";
        }
    };

    class DeadWall : public std::exception {
    public:
        const char* what() const noexcept override {
            return "Probably your client machine is located in country when your human rights are equal to zero :)";
        }
    };
}

namespace reactor::telegram {
        namespace error_codes {
            static constexpr const int32_t BadAuthorization = 401;
            static constexpr const int32_t NotFound = 404;
        }

        using TLId      = uint64_t;
        using TLDate    = uint64_t;
        using TLString  = std::string;   //TODO: Use multibyte UTF8 string, std::string for debug only

        template<typename _T>
        using TLOptional        = std::optional<_T>;
        using TLOptionalString  = TLOptional<TLString>;

        class Chat;
        class User;
        class Message;
        class Video;
        class MessageEntity;
        class Update;
        class Sticker;
        class ChatMember;
        class Server;
        class PhotoSize;
        class TLPollEngine;
        class ErrorHandler;

        using ChatPtr               = std::shared_ptr<Chat>;
        using UserPtr               = std::shared_ptr<User>;
        using MessagePtr            = std::shared_ptr<Message>;
        using UpdatePtr             = std::shared_ptr<Update>;
        using VideoPtr              = std::shared_ptr<Video>;
        using StickerPtr            = std::shared_ptr<Sticker>;
        using MessageEntityPtr      = std::shared_ptr<MessageEntity>;
        using ChatMemberPtr         = std::shared_ptr<ChatMember>;
        using PhotoSizePtr          = std::shared_ptr<PhotoSize>;
        using MessageEntitiesList   = std::list<MessageEntityPtr>;
        using TLPollEnginePtr       = std::unique_ptr<TLPollEngine>;

        using ServerPtr             = std::shared_ptr<Server>;
        using UpdatesList           = std::list<UpdatePtr>;
        using UsersList             = std::list<UserPtr>;

        class Chat {
        public:
            TLId id { 0 };
            TLString type;
            TLOptionalString title;
            TLOptionalString user_name;
            TLOptionalString first_name;
            TLOptionalString last_name;
        };

        class User {
        public:
            TLId id { 0 };
            bool is_bot;
            TLString first_name;
            TLOptionalString last_name;
            TLOptionalString username;
        };

        class Message {
        public:
            TLId message_id { 0 };
            TLOptional<UserPtr> from;
            TLDate date;
            ChatPtr chat;
            TLOptional<UserPtr> forward_from;
            TLOptional<MessagePtr> reply_to_message;
            TLOptionalString text;
            TLOptional<MessageEntitiesList> entities;
            TLOptional<StickerPtr> sticker;
            TLOptional<UsersList> new_chat_members;
            TLOptional<UserPtr> left_chat_member;
        };

        class MessageEntity {
        public:
            TLString type;
            uint32_t offset;
            uint32_t length;
            TLOptional<UserPtr> user;
            TLOptionalString url;

            static constexpr const char* BotCommand = "bot_command";
        };

        class Sticker {
        public:
            TLString file_id;
            int32_t width;
            int32_t height;
            bool is_animated;
            TLOptionalString emoji;
            TLOptionalString set_name;
        };

        class Video
        {
        public:
            TLString file_id;
            int32_t width;
            int32_t height;
            int32_t duration;
            TLOptional<PhotoSizePtr> thumb;
            TLOptionalString mime_type;
            TLOptional<int32_t> file_size;
        };

        class PhotoSize
        {
        public:
            TLString file_id;
            int32_t width;
            int32_t height;
            TLOptional<int32_t> file_size;
        };

        class ChatMember
        {
        public:
            UserPtr user;
            TLString status;
            TLOptional<int32_t> until_date;
            TLOptional<bool> can_be_edited;
            TLOptional<bool> can_post_messages;
            TLOptional<bool> can_edit_messages;
            TLOptional<bool> can_delete_messages;
            TLOptional<bool> can_restrict_members;
            TLOptional<bool> can_promote_members;
            TLOptional<bool> can_change_info;
            TLOptional<bool> can_invite_users;
            TLOptional<bool> can_pin_messages;
            TLOptional<bool> is_member;
            TLOptional<bool> can_send_messages;
            TLOptional<bool> can_send_media_messages;
            TLOptional<bool> can_send_polls;
            TLOptional<bool> can_send_other_messages;
            TLOptional<bool> can_add_web_page_previews;
        };

        class Update
        {
        public:
            TLId update_id { 0 };
            TLOptional<MessagePtr> message;
            TLOptional<MessagePtr> edited_message;
        };

        class ErrorHandler
        {
        public:
            static void processServerFailureByJsonRepresentation(const nlohmann::json& j);
        };

        struct BotCommand
        {
            std::string command {};
            size_t offset { 0 };
            size_t length { 0 };
        };
    }

namespace nlohmann {

    template <>
    struct adl_serializer<reactor::telegram::ChatPtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::ChatPtr& chat)
        {
            if (j.contains("ok") && !j["ok"].get<bool>())
                reactor::telegram::ErrorHandler::processServerFailureByJsonRepresentation(j);

            chat = std::make_shared<reactor::telegram::Chat>();
            chat->id = j["id"].get<decltype(chat->id)>();
            chat->type = j["type"].get<decltype(chat->type)>();

            if (auto titleIter = j.find("title"); titleIter != j.end())
                chat->title = titleIter->get<typename decltype(chat->title)::value_type>();

            if (auto userNameIter = j.find("user_name"); userNameIter != j.end())
                chat->user_name = userNameIter->get<typename decltype(chat->user_name)::value_type>();

            if (auto firstNameIter = j.find("first_name"); firstNameIter != j.end())
                chat->first_name = firstNameIter->get<typename decltype(chat->first_name)::value_type>();

            if (auto lastNameIter = j.find("last_name"); lastNameIter != j.end())
                chat->last_name = lastNameIter->get<typename decltype(chat->last_name)::value_type>();
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::UserPtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::UserPtr& user)
        {
            if (j.contains("ok") && !j["ok"].get<bool>())
                reactor::telegram::ErrorHandler::processServerFailureByJsonRepresentation(j);

            user = std::make_shared<reactor::telegram::User>();
            user->id = j["id"].get<decltype(user->id)>();
            user->is_bot = j["is_bot"].get<decltype(user->is_bot)>();
            user->first_name = j["first_name"].get<decltype(user->first_name)>();

            if (auto lastNameIter = j.find("last_name"); lastNameIter != j.end())
                user->last_name = lastNameIter->get<typename decltype(user->last_name)::value_type>();

            if (auto userNameIter = j.find("username"); userNameIter != j.end())
                user->username = userNameIter->get<typename decltype(user->username)::value_type>();
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::StickerPtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::StickerPtr& sticker)
        {
            if (j.contains("ok") && !j["ok"].get<bool>())
                reactor::telegram::ErrorHandler::processServerFailureByJsonRepresentation(j);

            sticker = std::make_shared<reactor::telegram::Sticker>();
            sticker->file_id = j["file_id"].get<decltype(sticker->file_id)>();
            sticker->width = j["width"].get<decltype(sticker->width)>();
            sticker->height = j["height"].get<decltype(sticker->height)>();
            sticker->is_animated = j["is_animated"].get<decltype(sticker->is_animated)>();

            if (auto emojiIter = j.find("emoji"); emojiIter != j.end())
                sticker->emoji = emojiIter->get<typename decltype(sticker->emoji)::value_type>();

            if (auto setNameIter = j.find("set_name"); setNameIter != j.end())
                sticker->set_name = setNameIter->get<typename decltype(sticker->set_name)::value_type>();
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::MessageEntityPtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::MessageEntityPtr& entity)
        {
            if (j.contains("ok") && !j["ok"].get<bool>())
                reactor::telegram::ErrorHandler::processServerFailureByJsonRepresentation(j);

            entity = std::make_shared<reactor::telegram::MessageEntity>();
            entity->type = j["type"].get<decltype(entity->type)>();
            entity->offset = j["offset"].get<decltype(entity->offset)>();
            entity->length = j["length"].get<decltype(entity->length)>();

            if (auto userIter = j.find("user"); userIter != j.end())
            {
                reactor::telegram::UserPtr userPtr = nullptr;
                nlohmann::adl_serializer<decltype(userPtr)>::from_json(*userIter, userPtr);
                entity->user = userPtr;
            }

            if (auto urlIter = j.find("url"); urlIter != j.end())
                entity->url = urlIter->get<typename decltype(entity->url)::value_type>();
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::MessagePtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::MessagePtr& message)
        {
            if (j.contains("ok") && !j["ok"].get<bool>())
                reactor::telegram::ErrorHandler::processServerFailureByJsonRepresentation(j);

            message = std::make_shared<reactor::telegram::Message>();

            message->message_id = j["message_id"].get<decltype(message->message_id)>();
            message->date = j["date"].get<decltype(message->date)>();
            nlohmann::adl_serializer<decltype(message->chat)>::from_json(j["chat"], message->chat);

            if (auto fromIter = j.find("from"); fromIter != j.end())
            {
                reactor::telegram::UserPtr userPtr = nullptr;
                nlohmann::adl_serializer<decltype(userPtr)>::from_json(*fromIter, userPtr);
                message->from = userPtr;
            }

            if (auto forwardFrom = j.find("forward_from"); forwardFrom != j.end())
            {
                reactor::telegram::UserPtr userPtr = nullptr;
                nlohmann::adl_serializer<decltype(userPtr)>::from_json(*forwardFrom, userPtr);
                message->forward_from = userPtr;
            }

            if (auto replyToMessageIter = j.find("reply_to_message"); replyToMessageIter != j.end())
            {
                reactor::telegram::MessagePtr messagePtr = nullptr;
                nlohmann::adl_serializer<decltype(messagePtr)>::from_json(*replyToMessageIter, messagePtr);
                message->reply_to_message = messagePtr;
            }

            if (auto textIter = j.find("text"); textIter != j.end())
                message->text = textIter->get<typename decltype(message->text)::value_type>();

            if (auto entitiesIter = j.find("entities"); entitiesIter != j.end())
            {
                if (!entitiesIter->is_array())
                    return;

                reactor::telegram::MessageEntitiesList entitiesList = {};
                nlohmann::adl_serializer<typename decltype(message->entities)::value_type>::from_json(*entitiesIter, entitiesList);
                message->entities = entitiesList;
            }

            if (auto stickerIter = j.find("sticker"); stickerIter != j.end())
            {
                reactor::telegram::StickerPtr sticker = nullptr;
                nlohmann::adl_serializer<typename decltype(message->sticker)::value_type>::from_json(*stickerIter, sticker);
                message->sticker = sticker;
            }

            if (auto newChatMembersIter = j.find("new_chat_members"); newChatMembersIter != j.end())
            {
                reactor::telegram::UsersList newChatMembersList = {};
                nlohmann::adl_serializer<typename decltype(message->new_chat_members)::value_type>::from_json(*newChatMembersIter, newChatMembersList);
                message->new_chat_members = newChatMembersList;
            }

            if (auto leftMemberIter = j.find("left_chat_member"); leftMemberIter != j.end())
            {
                reactor::telegram::UserPtr leftMember = nullptr;
                nlohmann::adl_serializer<typename decltype(message->left_chat_member)::value_type>::from_json(*leftMemberIter, leftMember);
                message->left_chat_member = leftMember;
            }
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::ChatMemberPtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::ChatMemberPtr& chatMember)
        {
            chatMember = std::make_shared<reactor::telegram::ChatMember>();

            nlohmann::adl_serializer<decltype(chatMember->user)>::from_json(j["user"], chatMember->user);
            chatMember->status = j["status"].get<decltype(chatMember->status)>();

            if (auto untilDateIter = j.find("until_date"); untilDateIter != j.end())
                chatMember->until_date = untilDateIter->get<typename decltype(chatMember->until_date)::value_type>();

            if (auto canBeEditedIter = j.find("can_be_edited"); canBeEditedIter != j.end())
                chatMember->can_be_edited = canBeEditedIter->get<typename decltype(chatMember->can_be_edited)::value_type>();

            if (auto iter = j.find("can_post_messages"); iter != j.end())
                chatMember->can_post_messages = iter->get<typename decltype(chatMember->can_post_messages)::value_type>();

            if (auto iter = j.find("can_edit_messages"); iter != j.end())
                chatMember->can_edit_messages = iter->get<typename decltype(chatMember->can_edit_messages)::value_type>();

            if (auto iter = j.find("can_delete_messages"); iter != j.end())
                chatMember->can_delete_messages = iter->get<typename decltype(chatMember->can_delete_messages)::value_type>();

            if (auto iter = j.find("can_restrict_members"); iter != j.end())
                chatMember->can_restrict_members = iter->get<typename decltype(chatMember->can_restrict_members)::value_type>();

            if (auto iter = j.find("can_promote_members"); iter != j.end())
                chatMember->can_promote_members = iter->get<typename decltype(chatMember->can_promote_members)::value_type>();

            if (auto iter = j.find("can_change_info"); iter != j.end())
                chatMember->can_change_info = iter->get<typename decltype(chatMember->can_change_info)::value_type>();

            if (auto iter = j.find("can_invite_users"); iter != j.end())
                chatMember->can_invite_users = iter->get<typename decltype(chatMember->can_invite_users)::value_type>();

            if (auto iter = j.find("can_pin_messages"); iter != j.end())
                chatMember->can_pin_messages = iter->get<typename decltype(chatMember->can_pin_messages)::value_type>();

            if (auto iter = j.find("is_member"); iter != j.end())
                chatMember->is_member = iter->get<typename decltype(chatMember->is_member)::value_type>();

            if (auto iter = j.find("can_send_messages"); iter != j.end())
                chatMember->can_send_messages = iter->get<typename decltype(chatMember->can_send_messages)::value_type>();

            if (auto iter = j.find("can_send_media_messages"); iter != j.end())
                chatMember->can_send_media_messages = iter->get<typename decltype(chatMember->can_send_media_messages)::value_type>();

            if (auto iter = j.find("can_send_polls"); iter != j.end())
                chatMember->can_send_polls = iter->get<typename decltype(chatMember->can_send_polls)::value_type>();

            if (auto iter = j.find("can_send_other_messages"); iter != j.end())
                chatMember->can_send_other_messages = iter->get<typename decltype(chatMember->can_send_other_messages)::value_type>();

            if (auto iter = j.find("can_add_web_page_previews"); iter != j.end())
                chatMember->can_add_web_page_previews = iter->get<typename decltype(chatMember->can_add_web_page_previews)::value_type>();
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::PhotoSizePtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::PhotoSizePtr& photoSize)
        {
            photoSize = std::make_shared<reactor::telegram::PhotoSize>();
            photoSize->file_id = j["file_id"].get<decltype(photoSize->file_id)>();
            photoSize->width = j["width"].get<decltype(photoSize->width)>();
            photoSize->height = j["height"].get<decltype(photoSize->height)>();

            if (auto fileSizeIter = j.find("file_size"); fileSizeIter != j.end())
            {
                photoSize->file_size = fileSizeIter->get<typename decltype(photoSize->file_size)::value_type>();
            }
        }
    };

    template <>
    struct adl_serializer<reactor::telegram::VideoPtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::VideoPtr& video)
        {
            video = std::make_shared<reactor::telegram::Video>();
            video->file_id = j["file_id"].get<decltype(video->file_id)>();
            video->width = j["width"].get<decltype(video->width)>();
            video->height = j["height"].get<decltype(video->height)>();
            video->duration = j["duration"].get<decltype(video->duration)>();

            if (auto thumbIter = j.find("thumb"); thumbIter != j.end())
            {
                reactor::telegram::PhotoSizePtr photoSize = nullptr;
                nlohmann::adl_serializer<typename decltype(video->thumb)::value_type>::from_json(*thumbIter, photoSize);
                video->thumb = photoSize;
            }

            if (auto mimeTypeIter = j.find("mime_type"); mimeTypeIter != j.end())
                video->mime_type = mimeTypeIter->get<typename decltype(video->mime_type)::value_type>();

            if (auto fileSizeIter = j.find("file_size"); fileSizeIter != j.end())
                video->file_size = fileSizeIter->get<typename decltype(video->file_size)::value_type>();

        }
    };

    template <>
    struct adl_serializer<reactor::telegram::UpdatePtr>
    {
        static void from_json(const nlohmann::json& j, reactor::telegram::UpdatePtr& update)
        {
            if (j.contains("ok") && !j["ok"].get<bool>())
                reactor::telegram::ErrorHandler::processServerFailureByJsonRepresentation(j);

            update = std::make_shared<reactor::telegram::Update>();
            update->update_id = j["update_id"].get<decltype(update->update_id)>();

            if (auto messageIter = j.find("message"); messageIter != j.end())
            {
                reactor::telegram::MessagePtr messagePtr = nullptr;
                nlohmann::adl_serializer<typename decltype(update->message)::value_type>::from_json(*messageIter, messagePtr);
                update->message = messagePtr;
            }

            if (auto editedMessageIter = j.find("edited_message"); editedMessageIter != j.end())
            {
                reactor::telegram::MessagePtr editedMessagePtr = nullptr;
                nlohmann::adl_serializer<typename decltype(update->edited_message)::value_type>::from_json(*editedMessageIter, editedMessagePtr);
                update->edited_message = editedMessagePtr;
            }
        }
    };

}

namespace reactor {
    namespace telegram {
        class ITelergamMessageProcessor {
        public:
            virtual ~ITelergamMessageProcessor() noexcept = default;

            virtual void onMessage(const telegram::MessagePtr& message, const telegram::ServerPtr& server) = 0;
            virtual void onBotCommands(const telegram::MessagePtr& message, const std::list<telegram::BotCommand>& commands, const telegram::ServerPtr& server) = 0;
            virtual void onMessageEdited(const telegram::MessagePtr& message, const telegram::ServerPtr& server) {}
        };

        struct TLAPI
        {
            static constexpr const char* getUpdates = "getUpdates";
            static constexpr const char* sendMessage = "sendMessage";
            static constexpr const char* getMe = "getMe";
            static constexpr const char* setChatTitle = "setChatTitle";
            static constexpr const char* sendVideo = "sendVideo";
        };

        class TLPollEngine
        {
            static constexpr const uint32_t UpdatesLimit = 256; ///< Max 256 updates per request
            static constexpr const uint32_t AwaitTimeout = 15; ///< Wait 15 seconds and sendMessage response

            friend class Server;

            class cURLDriver
            {
                CURL* m_curlInstance { nullptr };

                std::string m_proxyURI;
            public:
                using URL = std::string;
                using Parameters = std::unordered_map<std::string, std::string>;

                cURLDriver()
                {
                    curl_global_init(CURL_GLOBAL_DEFAULT);
                    m_curlInstance = curl_easy_init();
                }

                ~cURLDriver()
                {
                    if (m_curlInstance)
                    {
                        curl_easy_cleanup(m_curlInstance);
                        m_curlInstance = nullptr;
                    }
                }

                void setProxy(const std::string& proxyURI)
                {
                    m_proxyURI = proxyURI;
                }

                std::string performHttpRequestWithResultAsString(const URL& url, const Parameters& parameters)
                {
                    if (!m_proxyURI.empty())
                        curl_easy_setopt(m_curlInstance, CURLOPT_PROXY, m_proxyURI.c_str());

                    return TLPollEngine::cURLDriver::httpsRequest(m_curlInstance, url, parameters);
                }

                std::string performHttpRequestWithAttachedFile(const URL& url, const Parameters& parameters, const std::string& localFilePath)
                {
                    curl_httppost* formPost = nullptr;
                    curl_httppost* formEnd  = nullptr;

                    /**
                     * @note Cover this code with some 'pretty' abstraction layer.
                     * @todo Remove hardcoded tag 'video'. This function must support any type of contents!
                     */
                    curl_formadd(&formPost, &formEnd, CURLFORM_COPYNAME, "video", CURLFORM_CONTENTTYPE, "video/mpeg", CURLFORM_FILE, localFilePath.c_str(), CURLFORM_END);

                    for (const auto& [key, value] : parameters)
                    {
                        curl_formadd(&formPost, &formEnd, CURLFORM_COPYNAME, key.c_str(), CURLFORM_COPYCONTENTS, value.c_str(), CURLFORM_END);
                    }

                    curl_easy_setopt(m_curlInstance, CURLOPT_HTTPPOST, formPost);

                    return performHttpRequestWithResultAsString(url, {});
                }

                nlohmann::json performHttpRequestWithResultAsJson(const URL& url, const Parameters& parameters)
                {
                    return nlohmann::json::parse(performHttpRequestWithResultAsString(url, parameters));
                }

                void performHttpRequestWithoutResponse(const URL& url, const Parameters& parameters)
                {
                    if (!m_proxyURI.empty())
                        curl_easy_setopt(m_curlInstance, CURLOPT_PROXY, m_proxyURI.c_str());

                    TLPollEngine::cURLDriver::simpleHttpsRequest(m_curlInstance, url, parameters);
                }

            protected:
                struct MemoryChunk
                {
                    char* memory;
                    size_t size;
                };

                static constexpr const size_t DefaultChunkSize = 1;
                static constexpr const size_t ConnectionTimeout = 5; //5 seconds, after that time we will throw telegram::exceptions::DeadWall exception

                static std::string mapToParamsString(const Parameters& parameters)
                {
                    if (parameters.empty())
                        return "";

                    std::string result = "?";

                    for (const auto& [key, value] : parameters)
                    {
                        char* escaped_key   = curl_easy_escape(nullptr, key.c_str(), key.length());
                        char* escaped_value = curl_easy_escape(nullptr, value.c_str(), value.length());

                        result += fmt::format("{}={}&", escaped_key, escaped_value);

                        // Free chunks (that memory was allocated by cURL, we must free it here)
                        curl_free(escaped_key);
                        curl_free(escaped_value);
                    }

                    result.pop_back();  //remove last & character

                    return result;
                }

                static std::string httpsRequest(CURL* engine, const URL& url, const Parameters& parameters)
                {
                    cURLDriver::MemoryChunk buffer {
                            reinterpret_cast<char*>(std::malloc(cURLDriver::DefaultChunkSize)),     //allocate 1 byte
                            0x0 //no data here
                    };

                    const auto paramsString = cURLDriver::mapToParamsString(parameters);

                    curl_easy_setopt(engine, CURLOPT_SSL_VERIFYPEER, 0L);
                    curl_easy_setopt(engine, CURLOPT_SSL_VERIFYHOST, 0L);
                    curl_easy_setopt(engine, CURLOPT_CONNECTTIMEOUT, cURLDriver::ConnectionTimeout);
                    curl_easy_setopt(engine, CURLOPT_URL, (url + paramsString).c_str());
                    curl_easy_setopt(engine, CURLOPT_WRITEFUNCTION, cURLDriver::onWriteToMemoryRequest);
                    curl_easy_setopt(engine, CURLOPT_WRITEDATA, (void *)&buffer);
                    curl_easy_setopt(engine, CURLOPT_USERAGENT, "libcurl-agent/1.0"); // Maybe we should use here tag from configs?

                    spdlog::info("[cURLDriver::httpsRequest] perform GET request with await result to {}", (url + paramsString).c_str());

                    auto result = curl_easy_perform(engine);

                    if (result != CURLE_OK)
                    {
                        if (result == CURLE_OPERATION_TIMEOUTED)
                            throw telegram::exceptions::DeadWall();

                        throw std::runtime_error("[with-response] Failed to get response from url " + url);
                    }

                    std::string response { buffer.memory, buffer.size };
                    spdlog::info("[cURLDriver::httpsRequest] response \"{}\"", response);

                    curl_easy_reset(engine);
                    free(buffer.memory);

                    return response;
                }

                static void simpleHttpsRequest(CURL* engine, const URL& url, const Parameters& parameters)
                {
                    const auto paramsString = cURLDriver::mapToParamsString(parameters);

                    curl_easy_setopt(engine, CURLOPT_SSL_VERIFYPEER, 0L);
                    curl_easy_setopt(engine, CURLOPT_SSL_VERIFYHOST, 0L);
                    curl_easy_setopt(engine, CURLOPT_CONNECTTIMEOUT, cURLDriver::ConnectionTimeout);
                    curl_easy_setopt(engine, CURLOPT_URL, (url + paramsString).c_str());
                    curl_easy_setopt(engine, CURLOPT_USERAGENT, "libcurl-agent/1.0");

                    spdlog::info("[cURLDriver::simpleHttpsRequest] perform GET request to {}", (url + paramsString).c_str());

                    auto result = curl_easy_perform(engine);
                    curl_easy_reset(engine);

                    if (result != CURLE_OK)
                    {
                        if (result == CURLE_OPERATION_TIMEOUTED)
                            throw telegram::exceptions::DeadWall();

                        throw std::runtime_error("[simple] Failed to get response from url " + url);
                    }
                }

                static size_t onWriteToMemoryRequest(void* contents, size_t size, size_t nmemb, void* userp)
                {
                    size_t realsize = size * nmemb;
                    cURLDriver::MemoryChunk* mem = reinterpret_cast<cURLDriver::MemoryChunk*>(userp);

                    char* ptr = reinterpret_cast<char*>(std::realloc(mem->memory, mem->size + realsize + 1));
                    if(ptr == nullptr) {
                        /* out of memory! */
                        printf("not enough memory (realloc returned NULL)\n");
                        return 0;
                    }

                    /**
                     * @brief Worst 'allocator' function from cURL samples.
                     *        Better to use here std::string or fixed pre-initialized std::vector here
                     * @todo Rewrite this in future
                     */
                    mem->memory = ptr;
                    memcpy(&(mem->memory[mem->size]), contents, realsize);
                    mem->size += realsize;
                    mem->memory[mem->size] = 0;

                    return realsize;
                }
            };

            class TLOutcomingAction
            {
            public:
                virtual ~TLOutcomingAction() noexcept = default;

                virtual void onAction(const std::shared_ptr<cURLDriver>& driver) = 0;
            };

            void pushAction(const std::shared_ptr<TLOutcomingAction>& action)
            {
                m_actionsQueue.push(action);
            }

            class TLSendMessage : public TLOutcomingAction
            {
                telegram::ChatPtr m_chat;
                std::string m_text;
                std::string m_token;
            public:
                TLSendMessage(const telegram::ChatPtr& chat, const std::string& text, const std::string& token)
                    : m_chat(chat)
                    , m_text(text)
                    , m_token(token)
                {
                }

                void onAction(const std::shared_ptr<cURLDriver>& driver) override
                {
                    auto sendMessageApiUrl = fmt::format("https://api.telegram.org/bot{}/{}", m_token, TLAPI::sendMessage);
                    driver->performHttpRequestWithoutResponse(sendMessageApiUrl, {
                            { "chat_id", std::to_string(m_chat->id) },
                            { "text", m_text }
                    });
                }
            };

            class TLReplyMessage : public TLOutcomingAction
            {
                telegram::ChatPtr m_chat;
                telegram::MessagePtr m_messageToReply;
                std::string m_replyText;
                std::string m_token;

            public:
                TLReplyMessage(const telegram::ChatPtr& chat, const telegram::MessagePtr& replyMessage, const std::string& replyText, const std::string& token)
                    : m_chat(chat)
                    , m_messageToReply(replyMessage)
                    , m_replyText(replyText)
                    , m_token(token)
                {
                }

                void onAction(const std::shared_ptr<cURLDriver>& driver) override
                {
                    auto sendMessageApiUrl = fmt::format("https://api.telegram.org/bot{}/{}", m_token, TLAPI::sendMessage);
                    driver->performHttpRequestWithoutResponse(sendMessageApiUrl, {
                            { "chat_id", std::to_string(m_chat->id) },
                            { "text", m_replyText },
                            { "reply_to_message_id", std::to_string(m_messageToReply->message_id) }
                    });
                }
            };

            class TLSetChatTitle : public TLOutcomingAction
            {
                telegram::ChatPtr m_chat;
                std::string m_title;
                std::string m_token;
            public:
                TLSetChatTitle(const telegram::ChatPtr& chat, const std::string& title, const std::string& token)
                    : m_chat(chat)
                    , m_title(title)
                    , m_token(token)
                {
                }

                void onAction(const std::shared_ptr<cURLDriver>& driver) override
                {
                    auto sendMessageApiUrl = fmt::format("https://api.telegram.org/bot{}/{}", m_token, TLAPI::setChatTitle);
                    driver->performHttpRequestWithoutResponse(sendMessageApiUrl, {
                            { "chat_id", std::to_string(m_chat->id) },
                            { "title", m_title }
                    });
                }
            };

            class TLSendVideo : public TLOutcomingAction
            {
                telegram::ChatPtr m_chat;
                std::string m_filePath;
                std::string m_token;
            public:
                TLSendVideo(const telegram::ChatPtr& chat, const std::string& filePath, const std::string& token)
                    : m_chat(chat)
                    , m_filePath(filePath)
                    , m_token(token)
                {
                }

                void onAction(const std::shared_ptr<cURLDriver>& driver) override
                {
                    auto sendMessageApiUrl = fmt::format("https://api.telegram.org/bot{}/{}", m_token, TLAPI::sendVideo);

                    spdlog::info("[TLSendVideo::onAction] try to send video from file {}", m_filePath);
                    auto response = driver->performHttpRequestWithAttachedFile(sendMessageApiUrl, {
                            { "chat_id", std::to_string(m_chat->id) }
                    }, m_filePath);
                    spdlog::info("[TLSendVideo::onAction] response {}", response);
                }
            };

            std::shared_ptr<cURLDriver> m_curlDriver { nullptr };
            std::queue<std::shared_ptr<TLOutcomingAction>> m_actionsQueue {};
        public:
            using OnEventCallback = std::function<void(const UpdatesList&)>;

            explicit TLPollEngine(const std::string& telegramToken, const std::string& proxy)
                : m_token(telegramToken)
                , m_curlDriver(std::make_shared<cURLDriver>())
            {
                if (!proxy.empty())
                    m_curlDriver->setProxy(proxy);
            }

            void start(const OnEventCallback& callback, bool asDetachedThread = true)
            {
                m_updatesCallback = callback;
                m_isDead = false;

                checkToken();

                std::thread workerThread { std::bind(&TLPollEngine::workerProcedure, this) };
                if (asDetachedThread)
                    workerThread.detach();
                else
                    workerThread.join();
            }

            void stop()
            {
                m_isDead = true;
            }

            [[nodiscard]] bool isReadyToDestroy() const { return m_isDead; }

        private:
            void setTopUpdateId(TLId topId)
            {
                spdlog::info("[TLPollEngine::setTopUpdateId] change top update ID from {} to {}", m_lastUpdateId, topId);
                m_lastUpdateId = topId;
            }

            /**
             * @fn getUpdates
             * @brief gets all updates from telegram
             * @return list of updates
             * @note this method does not marks ready updates as 'read'. You must do that manually via method setTopUpdateId
             */
            UpdatesList getUpdates()
            {
                const std::string apiRequestUrl = fmt::format("https://api.telegram.org/bot{}/{}", m_token, TLAPI::getUpdates);

                auto httpResult = m_curlDriver->performHttpRequestWithResultAsJson(apiRequestUrl, {
                        {
                                { "offset", std::to_string(m_lastUpdateId) },
                                { "limit", std::to_string(TLPollEngine::UpdatesLimit) },
                                { "timeout", std::to_string(TLPollEngine::AwaitTimeout) }
                        }
                });

                UpdatesList result = {};

                const bool isOk = httpResult["ok"].get<bool>();
                if (!isOk)
                    telegram::ErrorHandler::processServerFailureByJsonRepresentation(httpResult);

                nlohmann::adl_serializer<UpdatesList>::from_json(httpResult["result"], result);

                return result;
            }

            /**
             * @fn checkToken
             * @brief Just try to retrive information about bot via method getMe
             * @throws telegram::exceptions::BadAuthorization when unable method getMe failed with error code 401
             * @throws telegram::exceptions::UnknownError when unable to detect error code (we don't know about passed code)
             */
            void checkToken()
            {
                spdlog::info("[TLPollEngine::checkToken] try to check telegram token ...");

                const std::string apiRequestUrl = fmt::format("https://api.telegram.org/bot{}/{}", m_token, TLAPI::getMe);

                auto httpResult = m_curlDriver->performHttpRequestWithResultAsJson(apiRequestUrl, {});

                const bool isOk = httpResult["ok"].get<bool>();
                if (!isOk)
                {
                    spdlog::critical("[TLPollEngine::checkToken] bad token! Shutdown ...");
                    telegram::ErrorHandler::processServerFailureByJsonRepresentation(httpResult);
                }
                else
                {
                    telegram::UserPtr me = nullptr;
                    nlohmann::adl_serializer<decltype(me)>::from_json(httpResult["result"], me);
                    spdlog::info("[TLPollEngine::checkToken] correct token! Bot id is {} with name {}", me->id, me->first_name);
                }
            }

            void workerProcedure()
            {
                /**
                 * @brief Bot main loop
                 *        Bot working by 3 stages : Take - Process - Ask
                 *        First of all bot will take all available updates from telegram server (maximum 256 updates)
                 *        After all our message processor will try to process all incoming updates.
                 *        Every process stage could spawn new action, who will be processed on the third stage
                 */
                while (!m_isDead)
                {
                    /**
                     * @brief STAGE 1 : GET UPDATES FROM TELEGRAM SERVER
                     */
                    auto updatesList = getUpdates();
                    if (updatesList.empty())
                        continue;   //skip current loop

                    spdlog::info("[TLPollEngine::workerProcedure] got {} new unprocessed updates from telegram", updatesList.size());

                    TLId topId = { m_lastUpdateId };

                    for (const auto& update : updatesList)
                    {
                        if (update->update_id > topId)
                        {
                            topId = update->update_id;
                        }
                    }

                    /**
                     * @brief STAGE 2 : USE LATEST UPDATE ID AS NEW HISTORY POINTER
                     */
                    setTopUpdateId(topId + 1); //new top update id must be greater by 1 than last top update id (look for crazy docs from tl team)
                    m_updatesCallback(updatesList);

                    /**
                     * @brief STAGE 3 : PROCESS OUTCOMING ACTIONS FROM BOT
                     */
                    if (!m_actionsQueue.empty())
                    {
                        spdlog::info("[TLPollEngine::workerProcedure] processing outcoming actions (total {})", m_actionsQueue.size());

                        size_t actionId = 1;
                        while (!m_actionsQueue.empty())
                        {
                            spdlog::info("[TLPollEngine::workerProcedure] processing outcoming action {} of {}", actionId, m_actionsQueue.size());

                            auto action = m_actionsQueue.front();   //take action
                            action->onAction(m_curlDriver); //perform it
                            m_actionsQueue.pop(); //remove from queue

                            ++actionId;
                        }
                    }
                }
            }

        private:
            std::string m_token;
            bool m_isDead { false };
            OnEventCallback m_updatesCallback;
            TLId m_lastUpdateId { 0 };
        };

        class Server : public std::enable_shared_from_this<Server> {
            TLPollEnginePtr m_pollEngine;
            ITelergamMessageProcessor* m_messageProcessor { nullptr };
            std::string m_token;
        public:
            Server(const std::string& token, ITelergamMessageProcessor* processor, const std::string& proxy = std::string())
                : m_pollEngine(std::make_unique<TLPollEngine>(token, proxy))
                , m_messageProcessor(processor)
                , m_token(token)
            {
                spdlog::info("[Server] start server with token \"{}\"", token);

                if (!proxy.empty())
                    spdlog::info("[Server] apply proxy {}", proxy);
            }

            void start(bool asDetached = true)
            {
                m_pollEngine->start(std::bind(&Server::onUpdates, this, std::placeholders::_1), asDetached);
            }

            void sendMessage(const telegram::ChatPtr& chat, const std::string& message)
            {
                m_pollEngine->pushAction(std::make_shared<TLPollEngine::TLSendMessage>(chat, message, m_token));
            }

            void replyMessage(const telegram::ChatPtr& chat, const telegram::MessagePtr& messageToReply, const std::string& replyText)
            {
                m_pollEngine->pushAction(std::make_shared<TLPollEngine::TLReplyMessage>(chat, messageToReply, replyText, m_token));
            }

            void setChatTitle(const telegram::ChatPtr& chat, const std::string& title)
            {
                m_pollEngine->pushAction(std::make_shared<TLPollEngine::TLSetChatTitle>(chat, title, m_token));
            }

            void sendVideo(const telegram::ChatPtr& chat, const std::string& pathToVideoFile)
            {
                m_pollEngine->pushAction(std::make_shared<TLPollEngine::TLSendVideo>(chat, pathToVideoFile, m_token));
            }
        private:
            void onUpdates(const UpdatesList& updates)
            {
                spdlog::info("[Server::onUpdates] got {} updates. Process it!", updates.size());

                for (const auto& update : updates)
                {
                    processUpdate(update);
                }
            }

            void processUpdate(const UpdatePtr& update)
            {
                if (update->message.has_value())
                {
                    auto message = (*update->message);

                    if (message->entities.has_value())
                    {
                        std::list<telegram::BotCommand> commands = {};

                        for (const auto& entity : (*message->entities))
                        {
                            if (entity->type == telegram::MessageEntity::BotCommand)
                            {
                                std::string command;

                                for (std::size_t charId = entity->offset; charId < entity->length; charId++)
                                {
                                    if ((*message->text)[charId] == '@')
                                        break; //stop the loop
                                    else
                                        command.push_back((*message->text)[charId]);
                                }

                                commands.push_back(telegram::BotCommand { command, entity->offset, entity->length });
                            }
                        }

                        /**
                         * @brief if we have any bot commands we must process it in other callback
                         */
                        if (!commands.empty())
                        {
                            spdlog::info("[Server::processUpdate] we have {} bot commands. Process it", commands.size());
                            m_messageProcessor->onBotCommands(message, commands, shared_from_this());
                            return;
                        }
                    }

                    spdlog::info("[Server::processUpdate] we haven't any bot commands. Process message in common callback");
                    m_messageProcessor->onMessage(message, shared_from_this());
                }

                if (update->edited_message.has_value())
                {
                    auto message = (*update->edited_message);

                    m_messageProcessor->onMessageEdited(message, shared_from_this());
                }

                /**
                 * @brief here we can process other types of message
                 */
            }
        };

        void telegram::ErrorHandler::processServerFailureByJsonRepresentation(const nlohmann::json& j)
        {
            int errorCode = j["error_code"].get<int>();

            if (errorCode == telegram::error_codes::BadAuthorization)
                throw telegram::exceptions::BadAuthorization();

            if (errorCode == telegram::error_codes::NotFound)
                throw telegram::exceptions::BotNotFound();

            /**
             * @brief Process other error codes here
             */

            throw telegram::exceptions::UnknownError(errorCode);
        }
    }

    namespace raptor {
#define BOT_CALLBACK(method) std::bind(&ChatBotMessageProcessor::method, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

        class ChatBotMessageProcessor : public telegram::ITelergamMessageProcessor {

            using BotCommandFunction = std::function<void(const telegram::MessagePtr&, const telegram::BotCommand&, const telegram::ServerPtr&)>;

            std::unordered_map<std::string, BotCommandFunction> m_commandRoutes;

        public:
            ChatBotMessageProcessor()
                : m_commandRoutes({
                                          { "/status", BOT_CALLBACK(onStatusRequest) },
                                          { "/auth", BOT_CALLBACK(onAuthorizationRequest) },
                                          { "/get_video", BOT_CALLBACK(onVideoRequest) }
                                  })
            {
            }

            void onMessage(const telegram::MessagePtr& message, const telegram::ServerPtr& server) override
            {
            }

            void onBotCommands(const telegram::MessagePtr& message, const std::list<telegram::BotCommand>& commands, const telegram::ServerPtr& server) override
            {
                if (!message->from.has_value())
                    return;

                for (const auto& command : commands)
                {
                    auto iter = m_commandRoutes.find(command.command);
                    if (iter == std::end(m_commandRoutes))
                    {
                        onBadCommand(message, command, server);
                    }
                    else
                    {
                        iter->second(message, command, server);
                    }
                }
            }

            void onMessageEdited(const telegram::MessagePtr& message, const telegram::ServerPtr& server) override
            {
                server->replyMessage(message->chat, message, "Я не буду обрабатывать это обновление!");
            }

        private:
            void onBadCommand(const telegram::MessagePtr& message, const telegram::BotCommand& command, const telegram::ServerPtr& server)
            {
                server->sendMessage(message->chat, fmt::format("Unknown command \"{}\".", command.command));
            }

            void onStatusRequest(const telegram::MessagePtr& message, const telegram::BotCommand& command, const telegram::ServerPtr& server)
            {
                /**
                 * @todo implement it
                 */
            }

            void onAuthorizationRequest(const telegram::MessagePtr& message, const telegram::BotCommand& command, const telegram::ServerPtr& server)
            {
                /**
                 * @todo implement it
                 */
            }

            void onVideoRequest(const telegram::MessagePtr& message, const telegram::BotCommand& command, const telegram::ServerPtr& server)
            {
                /**
                 * @todo implement it
                 */
            }
        };
    }

    int Application::run()
    {
        spdlog::info("Start telegram server ...");

        auto processor = new raptor::ChatBotMessageProcessor();

        auto testServer = std::make_shared<telegram::Server>("TOKEN", processor, "PROXY");
        testServer->start(false); //lock current thread
        delete processor;

        return 0;
    }
}