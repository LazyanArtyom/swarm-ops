# Design System

This client foundation uses a VS Code-inspired graphite design language with a small
Material-style accent set. The goal is quiet professional tooling: flat surfaces,
single-pixel separators, readable typography, and restrained color.

## Color Contract

Dark theme is the default reference theme:

| Role | Token | Color |
| --- | --- | --- |
| Primary text | `fg` | `#CCCCCC` |
| Muted text | `fg_muted` | `#8C8C8C` |
| Window/background | `bg_window` | `#1F2125` |
| Panel surface | `bg_surface` | `#24262A` |
| Separators | `border` | `#2B2B2B` |
| Primary accent | `primary` | `#4285F4` |

Light theme follows the same roles with a flat, low-shadow Material palette:

| Role | Token | Color |
| --- | --- | --- |
| Primary text | `fg` | `#202124` |
| Muted text | `fg_muted` | `#5F6368` |
| Window/background | `bg_window` | `#F3F4F6` |
| Panel surface | `bg_surface` | `#FFFFFF` |
| Separators | `border` | `#E0E3E7` |
| Primary accent | `primary` | `#1976D2` |

These anchor colors are covered by `app_theme_tests`. Changing them is a design
decision, not an incidental refactor.

## Token Structure

Theme tokens are grouped in `ui/theme/theme_colors.cpp`:

- `BasePaletteTokens`: text, surfaces, and borders.
- `IntentPaletteTokens`: primary, secondary, info, success, warning, and error roles.
- `InteractionPaletteTokens`: hover, pressed, selected, tooltip, and console metadata roles.

QSS should consume semantic tokens from `ThemeLoader::BuildTokens()` instead of
hardcoding colors. New widgets should first choose an existing semantic role. Add
a new token only when the role is reusable across multiple widgets.

## Styled Components

Shell styling should be expressed through a small set of reusable components
instead of broad descendant selectors.

- `panel-shell`: panel root surface
- `panel-header`: panel title bar
- `panel-content`: panel body surface
- `toolbar-chrome`: main toolbar chrome
- `console-header-controls`: compact controls embedded in the console header
- `settings-row`: horizontal form row used by settings pages

These roles are exposed by widgets through object names or `uiComponent`/role
properties. Prefer styling the component contract, not arbitrary descendants.
Avoid selectors like `QWidget#central_panel QWidget` because they flatten future
custom pages and make the shell hostile to extension.

## Layout And Borders

Panels are intentionally flat. Side panels and the console are separated from the
workspace by one-pixel splitter borders using `border`. Avoid nested framed
containers unless the border is part of the interaction model, such as an editor,
table, or explicit viewport.

## Typography

UI text uses Inter when bundled fonts load, then falls back to the platform UI
font. Console text uses JetBrains Mono when available, then the platform fixed
font. Font loading failures are logged through the central logger.
