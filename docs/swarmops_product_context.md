# SwarmOps Product Context

This repository is the Qt/C++ desktop client for SwarmOps. The architecture
specification in `../SwarmOps_architecture_specification_v3.docx` describes a
contract-first drone swarm platform where the GUI talks only to the API Gateway
over gRPC/protobuf.

## Architecture Direction

- The Qt client consumes only shared public contracts from the future
  `swarm-contracts` repository.
- Client-side handwritten code should stay local to this repository: generated
  API Gateway stubs are wrapped by Qt-friendly adapters that own deadlines,
  reconnects, authentication metadata, stream lifecycle, and signal/model
  translation.
- The GUI must not talk directly to Drone Gateway, Data Service, Kafka, or
  SwarmKit.
- Backend services use gRPC for commands and queries, and Kafka for telemetry,
  health, command result, alert, and replay-style event streams.
- Drone Gateway wraps SwarmKit and isolates drone transport details from the
  rest of the platform.
- Mission Workspace Service will own saved missions, background assets,
  graph/editing state, autosave, explicit checkpoints, mission-scoped presence,
  and coordinate transform metadata for telemetry overlays.

## Initial Client Implications

- Keep the current shell, settings, logging, translation, and packaging
  foundations clean and product-named.
- Build future networking as a client-local API Gateway wrapper rather than
  leaking generated gRPC types across the UI.
- Treat Open, Save, and workspace flows as backend-backed mission operations
  rather than local file operations once mission services are introduced.
