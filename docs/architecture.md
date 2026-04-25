# Architecture

This SwarmOps client intentionally keeps shell composition simple.

## Logging Boundary

The logging system currently keeps a process-global static facade in
[logger.h](../core/logging/logger.h).

That is a deliberate tradeoff for the SwarmOps client:

- simple app-wide logging API
- easy integration with Qt message handling
- low ceremony for desktop tools

For this client foundation, that is acceptable. Do not introduce an `ILogger`
abstraction by default just to remove the global facade. Add an injectable
logging interface only when a real caller needs multiple logger instances,
embedded use, or isolated test contexts.

## Command Metadata And Shell Composition

Commands expose metadata through `CommandMetadata` in
[command.h](../app/commands/command.h):

- command id
- title and description
- group id
- shortcut
- placement hints

That metadata exists to support:

- command discovery
- richer enable/disable logic
- future async commands
- future menu/toolbar generation if SwarmOps needs it

The SwarmOps client still keeps menu bar and toolbar composition manual in:

- [main_menu_bar.cpp](../ui/chrome/main_menu_bar.cpp)
- [main_tool_bar.cpp](../ui/chrome/main_tool_bar.cpp)

This is deliberate. Fully automatic shell generation would add complexity too
early and make the client foundation harder to read. For the SwarmOps client:

- command metadata is the semantic source of truth
- menu/toolbar wiring is explicit UI composition

SwarmOps can move to metadata-driven shell construction later when the command
surface grows enough to benefit from it.

## Page Session Ownership

`PageHost` owns page lifecycle and page session serialization:

- activate / deactivate
- close checks
- page state save / restore
- tab or single-page session state

`CentralPanel` exposes that capability as a thin wrapper, and
`ShellLayoutManager` persists the page session beside window layout state.

This keeps:

- page state with the workspace layer
- shell geometry with the shell layer
- startup/restore orchestration with the shell layout manager
