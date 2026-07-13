---
name: Obsidian Industrial
colors:
  surface: '#131313'
  surface-dim: '#131313'
  surface-bright: '#393939'
  surface-container-lowest: '#0e0e0e'
  surface-container-low: '#1c1b1b'
  surface-container: '#201f1f'
  surface-container-high: '#2a2a2a'
  surface-container-highest: '#353534'
  on-surface: '#e5e2e1'
  on-surface-variant: '#bbcabf'
  inverse-surface: '#e5e2e1'
  inverse-on-surface: '#313030'
  outline: '#86948a'
  outline-variant: '#3c4a42'
  surface-tint: '#4edea3'
  primary: '#4edea3'
  on-primary: '#003824'
  primary-container: '#10b981'
  on-primary-container: '#00422b'
  inverse-primary: '#006c49'
  secondary: '#4cd7f6'
  on-secondary: '#003640'
  secondary-container: '#03b5d3'
  on-secondary-container: '#00424e'
  tertiary: '#ffb3ad'
  on-tertiary: '#68000a'
  tertiary-container: '#ff7a73'
  on-tertiary-container: '#79000e'
  error: '#ffb4ab'
  on-error: '#690005'
  error-container: '#93000a'
  on-error-container: '#ffdad6'
  primary-fixed: '#6ffbbe'
  primary-fixed-dim: '#4edea3'
  on-primary-fixed: '#002113'
  on-primary-fixed-variant: '#005236'
  secondary-fixed: '#acedff'
  secondary-fixed-dim: '#4cd7f6'
  on-secondary-fixed: '#001f26'
  on-secondary-fixed-variant: '#004e5c'
  tertiary-fixed: '#ffdad7'
  tertiary-fixed-dim: '#ffb3ad'
  on-tertiary-fixed: '#410004'
  on-tertiary-fixed-variant: '#930013'
  background: '#131313'
  on-background: '#e5e2e1'
  surface-variant: '#353534'
typography:
  headline-lg:
    fontFamily: Geist
    fontSize: 40px
    fontWeight: '700'
    lineHeight: 48px
    letterSpacing: -0.02em
  headline-lg-mobile:
    fontFamily: Geist
    fontSize: 32px
    fontWeight: '700'
    lineHeight: 40px
    letterSpacing: -0.02em
  headline-md:
    fontFamily: Geist
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
  body-lg:
    fontFamily: Geist
    fontSize: 16px
    fontWeight: '400'
    lineHeight: 24px
  body-md:
    fontFamily: Geist
    fontSize: 14px
    fontWeight: '400'
    lineHeight: 20px
  label-md:
    fontFamily: JetBrains Mono
    fontSize: 12px
    fontWeight: '500'
    lineHeight: 16px
    letterSpacing: 0.05em
  label-sm:
    fontFamily: JetBrains Mono
    fontSize: 10px
    fontWeight: '500'
    lineHeight: 14px
    letterSpacing: 0.05em
rounded:
  sm: 0.125rem
  DEFAULT: 0.25rem
  md: 0.375rem
  lg: 0.5rem
  xl: 0.75rem
  full: 9999px
spacing:
  base: 4px
  gutter: 16px
  margin-mobile: 16px
  margin-desktop: 32px
  max-width: 1440px
---

## Brand & Style
This design system is built on a foundation of "Obsidian Industrial," a high-performance aesthetic designed for technical environments, data visualization, and engineering interfaces. The brand personality is precise, authoritative, and rugged. It targets developers, data scientists, and power users who require high focus and low eye strain.

The design style is a hybrid of **Minimalism** and **Modern Corporate**, utilizing heavy whitespace (through spacing, not color) and sharp typographic hierarchy. It evokes an emotional response of reliability and "mission-critical" stability. The interface feels like a high-end physical console—intentional, structured, and uncompromising.

## Colors
The palette is rooted in a neutral dark charcoal gray (`#121212`) to eliminate color bias in data interpretation. All surfaces utilize monochromatic shifts of gray to define depth.

High-vibrancy accents provide functional signaling:
- **Emerald Green (#10b981):** Primary action and "Success" states.
- **Cyan (#06b6d4):** Information, links, and secondary interactive cues.
- **Red (#ef4444):** Critical errors, destructive actions, and alerts.

Text is rendered in varying opacities of white: High Emphasis (90%), Medium Emphasis (60%), and Disabled (38%).

## Typography
The system uses **Geist** for its neutral, technical clarity and exceptional legibility in dark environments. For data points, code blocks, and metadata, **JetBrains Mono** is employed to provide a rhythmic, industrial feel that differentiates static content from dynamic or technical values.

Typographic scale is tight and efficient. All labels use the monospaced font in uppercase with slight letter spacing to reinforce the "instrument panel" aesthetic.

## Layout & Spacing
The layout follows a **Fixed Grid** philosophy on desktop (12 columns) and a **Fluid Grid** on mobile (4 columns). The rhythm is based on a 4px baseline shift to ensure mathematical precision in element alignment.

- **Desktop:** 1440px max-width, centered, with 32px side margins and 16px gutters.
- **Tablet:** Fluid width with 24px margins.
- **Mobile:** Fluid width with 16px margins.

Density is high. Padding within components is kept minimal to maximize information density, typical of professional dashboards and IDEs.

## Elevation & Depth
Elevation is expressed through **Tonal Layers** rather than soft shadows. In this dark charcoal environment, depth is achieved by lightening the surface color.

1.  **Level 0 (Background):** #121212 (Main canvas).
2.  **Level 1 (Cards/Panels):** #1e1e1e.
3.  **Level 2 (Modals/Popovers):** #2a2a2a.

**Low-contrast outlines** are used exclusively to define boundaries. Borders should be 1px solid `#333333`. Interactivity is signaled by "glowing" borders using the Primary or Secondary accent colors with a subtle 4px outer blur.

## Shapes
The shape language is **Soft** but disciplined. A 0.25rem (4px) border radius is applied to standard components like inputs and buttons to retain a sense of precision. Larger containers like cards use 0.5rem (8px). This creates a "machined" look—deliberately not sharp enough to be aggressive, but not rounded enough to feel consumer-casual.

## Components
- **Buttons:** Primary buttons are solid Emerald Green (#10b981) with black text. Secondary buttons are outlined in Cyan (#06b6d4) with Cyan text.
- **Inputs:** Dark backgrounds (#1e1e1e) with a 1px border (#333333). On focus, the border transitions to Cyan (#06b6d4) with a subtle inner glow.
- **Chips/Tags:** Monospaced labels in JetBrains Mono. Success tags use a 10% opacity Emerald Green fill with a solid Emerald border.
- **Lists:** High-density rows (32px or 40px height). Hover states use a subtle #2a2a2a background shift.
- **Cards:** No shadows. Defined by a #333333 border and #1e1e1e fill.
- **Data Tables:** Strict 1px horizontal dividers. Header cells use `label-sm` in JetBrains Mono for a technical, tabular feel.