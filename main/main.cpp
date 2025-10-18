#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include <regex>

using json = nlohmann::json;

class Bot {
public:
    struct Message {
        std::string chat_id;
        std::string text;
    };

    struct Callback {
        std::string chat_id;
        std::string data;
        std::string callback_id;
        int message_id;
    };

    std::string token;
    std::string baseUrl;
    long long int last_update_id = 0;
    std::vector<Message> receivedMessages;
    std::vector<Callback> receivedCallbacks;

    Bot(const std::string& botToken) : token(botToken) {
        baseUrl = "https://api.telegram.org/bot" + token;
        curl_global_init(CURL_GLOBAL_ALL);
    }
    
    ~Bot() {
        curl_global_cleanup();
    }

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
        size_t totalSize = size * nmemb;
        output->append((char*)contents, totalSize);
        return totalSize;
    }

    void sendMessage(const std::string& chat_id, const std::string& text) {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = baseUrl + "/sendMessage";
            json payload = {
                {"chat_id", chat_id},
                {"text", text}
            };
            std::string payloadStr = payload.dump();
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "sendMessage Curl error: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
    }

    void sendGlassBtnMessage(const std::string& chat_id, const std::string& text, const std::vector<std::pair<std::string, std::string>>& buttons) {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = baseUrl + "/sendMessage";
            json keyboard = { {"inline_keyboard", json::array()} };
            json row = json::array();
            for (const auto& btn : buttons) {
                row.push_back({ {"text", btn.first}, {"callback_data", btn.second} });
            }
            keyboard["inline_keyboard"].push_back(row);
            json payload = {
                {"chat_id", chat_id},
                {"text", text},
                {"reply_markup", keyboard}
            };
            std::string payloadStr = payload.dump();
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "sendGlassBtnMessage Curl error: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
    }

    void answerCallbackQuery(const std::string& callback_id, const std::string& text) {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = baseUrl + "/answerCallbackQuery";
            json payload = {
                {"callback_query_id", callback_id},
                {"text", text},
                {"show_alert", false}
            };
            std::string payloadStr = payload.dump();
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "answerCallbackQuery Curl error: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
    }

    void editMessageText(const std::string& chat_id, int message_id, const std::string& new_text) {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string url = baseUrl + "/editMessageText";
            json payload = {
                {"chat_id", chat_id},
                {"message_id", message_id},
                {"text", new_text}
            };
            std::string payloadStr = payload.dump();
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payloadStr.c_str());
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                std::cerr << "editMessageText Curl error: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
            curl_slist_free_all(headers);
        }
    }

    void fetchUpdatesOnce() {
        CURL* curl = curl_easy_init();
        if (curl) {
            std::string response;
            std::string getUpdatesUrl = baseUrl + "/getUpdates?offset=" + std::to_string(last_update_id + 1);
            curl_easy_setopt(curl, CURLOPT_URL, getUpdatesUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            CURLcode res = curl_easy_perform(curl);
            if (res == CURLE_OK) {
                try {
                    json j = json::parse(response);
                    if (!j.contains("result")) return;
                    for (auto& update : j["result"]) {
                        last_update_id = update["update_id"].get<long long>();
                        if (update.contains("callback_query")) {
                            auto cb = update["callback_query"];
                            std::string data = cb["data"];
                            std::string callback_id = cb["id"];
                            std::string chat_id = std::to_string(cb["message"]["chat"]["id"].get<long long>());
                            int message_id = cb["message"]["message_id"].get<int>();
                            receivedCallbacks.push_back({ chat_id, data, callback_id, message_id });
                        }
                        if (update.contains("message") && update["message"].contains("text")) {
                            auto msg = update["message"];
                            std::string text = msg["text"];
                            std::string chat_id = std::to_string(msg["chat"]["id"].get<long long>());
                            receivedMessages.push_back({ chat_id, text });
                        }
                    }
                } catch (std::exception& e) {
                    std::cerr << "JSON parsing error: " << e.what() << " Response: " << response << std::endl;
                }
            } else {
                std::cerr << "fetchUpdatesOnce Curl error: " << curl_easy_strerror(res) << std::endl;
            }
            curl_easy_cleanup(curl);
        }
    }
};

enum class UserState {
    Idle,               
    WaitingForNewMessage,
    INGAME
};

struct BotUser{
    std::string name;
    std::string id;
    int coins;
    long int scores;
    UserState state;
    
    BotUser(std::string Id, std::string Name) 
        : id(Id), name(Name), coins(0), scores(0), state(UserState::Idle) {}
    
    BotUser() : id(""), name("بازیکن"), coins(0), scores(0), state(UserState::Idle) {}
};

struct BotPlayer{
    BotUser user;
    std::string role;
    
    BotPlayer(BotUser User, std::string Role):user(User),role(Role){}
    BotPlayer() : user(), role("") {}
};

bool isValidFarsiName(const std::string& name) {
    if (name.length() < 3 || name.length() > 15) {
        return false;
    }

    std::regex farsiRegex(u8"^[\u0600-\u06FF]+$");
    return std::regex_match(name, farsiRegex);
}


int main() {
    Bot bot("8262579615:AAE97Hz7u-Qa0oUghu4JdfvR6xw2PbxipMU"); 
    std::map<std::string,BotUser> users;
    std::map<std::string,BotPlayer> players;

    while (true) {
        bot.fetchUpdatesOnce();
        
        for (const auto& msg : bot.receivedMessages) {
            std::string chat_id = msg.chat_id;
            std::string text = msg.text;
            
            if (users.count(chat_id) && users[chat_id].state == UserState::WaitingForNewMessage) {
                
                if (isValidFarsiName(text)) {
                    users[chat_id].name = text;
                    users[chat_id].state = UserState::Idle;
                    bot.sendMessage(chat_id, "✅ نام شما با موفقیت به " + text + " تغییر یافت.");
                } else {
                    bot.sendMessage(chat_id, "❌ نام وارد شده نامعتبر است. لطفا فقط از حروف فارسی (بین 3 تا 15 حرف) استفاده کنید. دوباره تلاش کنید:");
                }
                
            } 
            else if (text == "/start") {
                if(users.count(chat_id)){
                    bot.sendMessage(chat_id,"سلام👋 " + users[chat_id].name);
                } else {
                    bot.sendMessage(chat_id,"سلام👋\nبه ربات بازی سیلم خوش آمدید🌹\nمیتوانید با دستور /profile و تغییر نام،اقدام به تغییر نام خود کنید👤");
                    users.emplace(chat_id, BotUser(chat_id, "بازیکن"));
                }
            }
            else if(text == "/profile"){
                std::string profile = "پروفایل بازیکن👤\n\n💢 آیدی: " + msg.chat_id + "\n✏ نام: " + users[msg.chat_id].name + "\n💰 سکه: " + std::to_string(users[msg.chat_id].coins) + "\n⭐ امتیاز: " + std::to_string(users[msg.chat_id].scores);
                bot.sendGlassBtnMessage(chat_id,profile,{{"تغییر نام", "changeName"},{"تنظیمات بیشتر", "setting"}});
            }
            else if(text == "/startgame"){
                users[chat_id].state = UserState::INGAME;
                BotPlayer player(users[chat_id],"doctor");
                players[chat_id] = player;
            }
            else {
                if(users[chat_id].state == UserState::INGAME){         
                    if(users[chat_id].id == chat_id){
                        bot.sendMessage(users[chat_id].id,users[chat_id].name + ": " +text);
                    }
                    
                }else{
                        bot.sendMessage(chat_id,text + "؟");
                     }
            }
        }

        for (const auto& cb : bot.receivedCallbacks) {
            if (cb.data == "changeName") {
                if(users.count(cb.chat_id)){
                    users[cb.chat_id].state = UserState::WaitingForNewMessage;
                    bot.sendMessage(cb.chat_id, "👤 لطفا یک نام فارسی بین 3 تا 15 حرف انتخاب کنید. (فقط حروف فارسی، بدون عدد و شکلک)");
                    bot.answerCallbackQuery(cb.callback_id,"در حال تغییر نام");
                }
            } else if (cb.data == "setting") {
                bot.editMessageText(cb.chat_id, cb.message_id, "شما در تنظیمات هستید:");
                bot.answerCallbackQuery(cb.callback_id,"تنظیمات بیشتر");
            }
        }

        bot.receivedMessages.clear();
        bot.receivedCallbacks.clear();
        sleep(1);
    }

    return 0;
}
