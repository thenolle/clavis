# Contributing to Clavis

## Overview
Clavis is a native Windows application focused on simplicity, performance, and predictable behavior.

## Development rules
- Keep code minimal and readable
- Avoid unnecessary dependencies
- Maintain Win32 native performance
- Ensure hooks remain stable and safe

## Pull request process
1. Fork repository
2. Create feature branch
3. Make changes
4. Test on Windows
5. Submit PR with template filled

## Code style
- Clear naming
- No over-engineering
- Prefer explicit logic over abstraction

## Soundpack changes
- Must remain backward compatible
- `config.json` format must not break existing packs

## Notes
Changes affecting hooks, audio pipeline, or hotkeys require careful testing.