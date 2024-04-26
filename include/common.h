#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H
#include <string>
namespace chat_common{
struct SavedMessage {
    std::string name;
    std::string message;
};
}
#endif