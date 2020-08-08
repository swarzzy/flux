#pragma once
#include "Common.h"
#include "flux_console_commands.h"
#include "flux_flat_array.h"

struct LoggerBufferBlock {
    static const u32 Size = 4096;
    static const u32 Capacity = Size - 1;
    LoggerBufferBlock* next;
    LoggerBufferBlock* prev;
    u32 at;
    char data[Size];
};

struct Logger {
    AllocateFn* allocator;
    DeallocateFn* free;
    void* allocatorData;
    FlatArray<char> buffer;
    char formatBuffer[4096];
};

void InitLogger(Logger* logger, AllocateFn* allocator, DeallocateFn free, void* allocatorData);
void ClearLogger(Logger* logger);

void LoggerPushString(Logger* logger, const char* string);
void LogMessage(Logger* logger, const char* fmt, ...);
void LogMessageAPI(void* loggerData, const char* fmt, va_list* args);


struct ConsoleCommandArgs {
    char* args;
};

const char* PullCommandArg(ConsoleCommandArgs* args);

typedef void(ConsoleCommandFn)(Console* console, Context* gameContext, ConsoleCommandArgs* args);

struct ConsoleCommand {
    const char* name;
    ConsoleCommandFn* command;
    const char* description;
};

static const ConsoleCommand GlobalConsoleCommands[] = {
    { "clear",              ConsoleClearCommand },
    { "help",               ConsoleHelpCommand },
    { "history",            ConsoleHistoryCommand },
    { "echo",               ConsoleEchoCommand },

    { "recompile_shaders",  RecompileShadersCommand },
    { "toggle_dbg_overlay", ToggleDebugOverlayCommand },
    { "load", LoadCommand }
};

struct ConsoleCommandRecord {
    ConsoleCommandRecord* next;
    ConsoleCommandRecord* prev;
    char* string;
};

struct Console {
    Context* gameContext;
    Logger* logger;
    AllocateFn* allocate;
    void* allocatorData;
    bool autoScrollEnabled;
    bool justOpened;
    u32 commandHistoryCount;
    // TODO: We don't need to store history as a linked list when using malloc
    // It could be just flat array
    ConsoleCommandRecord* commandHistoryCursor;
    ConsoleCommandRecord* firstCommandRecord;
    ConsoleCommandRecord* lastCommandRecord;
    char inputBuffer[256];
};

void InitConsole(Console* console, Logger* logger, AllocateFn* allocate, void* allocatorData, Context* gameContext);
void DrawConsole(Console* console);
