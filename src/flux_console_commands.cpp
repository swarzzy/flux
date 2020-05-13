#include "flux_console_commands.h"
#include "flux_console.h"

#define REGISTER_SET_FLOAT(name, variable) \
if (StringsAreEqual(varName, name)) {\
    variableRecognized = true;                          \
    const char* varValue = PullCommandArg(args);\
    if (varValue) {\
        auto parseResult = StringToFloat(varValue);\
        if (parseResult.succeed) {\
            LogMessage(console->logger, "Setting a value of %s to %f. Old value was %f\n", name, parseResult.value, variable);\
            variable = parseResult.value;\
            return;\
        } else {\
            LogMessage(console->logger, "Failed to parse a value\n");\
        }\
    } else {\
        LogMessage(console->logger, "No value specified\n");\
    }\
} do {} while(false)\

#define REGISTER_SET_IV3(name, variable) \
if (StringsAreEqual(varName, name)) {       \
    variableRecognized = true;                          \
    const char* varValue1 = PullCommandArg(args);       \
    const char* varValue2 = PullCommandArg(args);       \
    const char* varValue3 = PullCommandArg(args);       \
    if (varValue1 && varValue2 && varValue3) {\
        auto parseResult1 = StringToInt(varValue1);\
        auto parseResult2 = StringToInt(varValue2);\
        auto parseResult3 = StringToInt(varValue3);\
        if (parseResult1.succeed && parseResult2.succeed && parseResult3.succeed) {\
            LogMessage(console->logger, "Setting a value of %s to (%d, %d, %d). Old value was (%d, %d, %d)\n", name, (int)parseResult1.value, (int)parseResult2.value, (int)parseResult2.value, (int)variable.x, (int)variable.y, (int)variable.z); \
            variable.x = parseResult1.value;\
            variable.y = parseResult2.value;\
            variable.z = parseResult3.value;\
            return;\
        } else {\
            LogMessage(console->logger, "Failed to parse a value\n");\
        }\
    } else {\
        LogMessage(console->logger, "No value specified\n");\
    }\
} do {} while(false)\


void ConsoleClearCommand(Console* console, Context* context, ConsoleCommandArgs* args) {
    ClearLogger(console->logger);
}

void ConsoleHelpCommand(Console* console, Context* context, ConsoleCommandArgs* args) {
    if (args->args) {
        bool found = false;
        for (u32x i = 0; i < array_count(GlobalConsoleCommands); i++) {
            auto command = GlobalConsoleCommands + i;
            if (MatchStrings(command->name, args->args)) {
                found = true;
                LogMessage(console->logger, "%s", command->name);
                if (command->description) {
                    LogMessage(console->logger, " - %s\n", command->description);
                } else {
                    LogMessage(console->logger, "\n");
                }
            }
        }
        if (!found) {
            LogMessage(console->logger, "Can't help you. Commad %s is not exist\n", args->args);
        }
    } else {
        LogMessage(console->logger, "Available commands are:\n");
        for (u32x i = 0; i < array_count(GlobalConsoleCommands); i++) {
            auto command = GlobalConsoleCommands + i;
            LogMessage(console->logger, "%s\n", command->name);
        }
    }
}

void ConsoleHistoryCommand(Console* console, Context* context, ConsoleCommandArgs* args) {
    auto logger = console->logger;
    LogMessage(logger, "History of commands:\n");
    auto record = console->firstCommandRecord;
    while(record) {
        LogMessage(logger, "%s\n", record->string);
        record = record->next;
    }
}

void ConsoleEchoCommand(Console* console, Context* context, ConsoleCommandArgs* args) {
    auto logger = console->logger;
    if (args->args) {
        LogMessage(logger, "%s\n", args->args);
    }
}

void RecompileShadersCommand(Console* console, Context* context, ConsoleCommandArgs* args) {
    RecompileShaders(context->renderer);
}

void ToggleDebugOverlayCommand(Console* console, Context* context, ConsoleCommandArgs* args) {
    GlobalDrawDebugOverlay = !GlobalDrawDebugOverlay;
}
