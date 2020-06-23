#pragma once

struct Context;
struct Console;
struct ConsoleCommandArgs;

void ConsoleClearCommand(Console* console, Context* context, ConsoleCommandArgs* args);
void ConsoleHelpCommand(Console* console, Context* context, ConsoleCommandArgs* args);
void ConsoleHistoryCommand(Console* console, Context* context, ConsoleCommandArgs* args);
void ConsoleEchoCommand(Console* console, Context* context, ConsoleCommandArgs* args);

void RecompileShadersCommand(Console* console, Context* context, ConsoleCommandArgs* args);
void ToggleDebugOverlayCommand(Console* console, Context* context, ConsoleCommandArgs* args);
void LoadCommand(Console* console, Context* context, ConsoleCommandArgs* args);
