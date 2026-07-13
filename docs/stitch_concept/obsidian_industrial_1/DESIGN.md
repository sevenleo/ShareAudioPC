---
name: Obsidian Industrial
colors:
  surface: '#0e1511'
  surface-dim: '#0e1511'
  surface-bright: '#343b36'
  surface-container-lowest: '#09100c'
  surface-container-low: '#161d19'
  surface-container: '#1a211d'
  surface-container-high: '#242c27'
  surface-container-highest: '#2f3632'
  on-surface: '#dde4dd'
  on-surface-variant: '#bbcabf'
  inverse-surface: '#dde4dd'
  inverse-on-surface: '#2b322d'
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
  tertiary: '#ffb95f'
  on-tertiary: '#472a00'
  tertiary-container: '#e29100'
  on-tertiary-container: '#523200'
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
  tertiary-fixed: '#ffddb8'
  tertiary-fixed-dim: '#ffb95f'
  on-tertiary-fixed: '#2a1700'
  on-tertiary-fixed-variant: '#653e00'
  background: '#0e1511'
  on-background: '#dde4dd'
  surface-variant: '#2f3632'
typography:
  display-lg:
    fontFamily: Inter
    fontSize: 48px
    fontWeight: '700'
    lineHeight: 56px
    letterSpacing: -0.02em
  headline-lg:
    fontFamily: Inter
    fontSize: 32px
    fontWeight: '600'
    lineHeight: 40px
    letterSpacing: -0.01em
  headline-lg-mobile:
    fontFamily: Inter
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
  headline-md:
    fontFamily: Inter
    fontSize: 24px
    fontWeight: '600'
    lineHeight: 32px
  body-lg:
    fontFamily: Inter
    fontSize: 18px
    fontWeight: '400'
    lineHeight: 28px
  body-md:
    fontFamily: Inter
    fontSize: 16px
    fontWeight: '400'
    lineHeight: 24px
  label-md:
    fontFamily: Inter
    fontSize: 14px
    fontWeight: '500'
    lineHeight: 20px
    letterSpacing: 0.01em
  label-sm:
    fontFamily: Inter
    fontSize: 12px
    fontWeight: '600'
    lineHeight: 16px
    letterSpacing: 0.05em
rounded:
  sm: 0.25rem
  DEFAULT: 0.5rem
  md: 0.75rem
  lg: 1rem
  xl: 1.5rem
  full: 9999px
spacing:
  base: 4px
  xs: 4px
  sm: 8px
  md: 16px
  lg: 24px
  xl: 40px
  gutter: 24px
  margin-mobile: 16px
  margin-desktop: 48px
---

## Brand & Style

The design system is engineered for professional, high-performance environments where clarity and focus are paramount. It adopts a **Modern Industrial** aesthetic, characterized by a deep, neutral foundation that eliminates visual noise and directs attention toward critical data and actions.

The brand personality is authoritative, precise, and utilitarian. It targets technical professionals who require a high-contrast environment to mitigate eye strain during extended use. The emotional response is one of stability and control, evoked through a disciplined use of space and a "dark-first" color philosophy. The style utilizes subtle borders and monochromatic layering to create a sense of structural integrity without the softness of consumer-grade minimalism.

## Colors

The palette transitions from a standard "deep blue" dark mode to a true **Charcoal/Gray-Black** scale to maximize contrast. 

- **Neutral Base:** The background uses a neutral charcoal (`#0F1115`), moving away from blue tints to ensure semantic colors do not bleed into the surroundings.
- **Primary (Emerald):** A vibrant emerald green used for primary actions and success states. It is calibrated for high luminosity against the dark base.
- **Secondary (Cyan):** A piercing cyan used for information, selection states, and supportive highlights.
- **Semantic Accents:** Amber is reserved for warnings and Red for critical errors. Both are pushed to high saturation to ensure "pop" and immediate recognition.
- **Text:** Primary text is a crisp White-Gray (`#F9FAFB`), while secondary text uses a muted Steel Gray (`#9CA3AF`) to maintain hierarchy.

## Typography

This design system utilizes **Inter** exclusively to maintain a systematic, utilitarian appearance. The type scale is optimized for legibility in dense data environments.

- **Headlines:** Use tighter letter-spacing and heavier weights to create a strong visual anchor.
- **Body:** Standardized at 16px for optimal readability against dark backgrounds.
- **Labels:** Small labels utilize a slight tracking increase and uppercase transform for clear categorization and "meta" information display.
- **Mobile scaling:** Display and large headlines drop significantly in size on mobile to prevent awkward line breaks while maintaining bold weights.

## Layout & Spacing

The spacing philosophy follows a strict **4px baseline grid**. 

- **Grid System:** A 12-column fluid grid is used for desktop (breakpoint 1280px+), transitioning to an 8-column grid for tablets (768px - 1279px) and a 4-column grid for mobile.
- **Content Density:** High-density layouts are preferred. Use `16px` (md) as the standard padding for containers and `24px` (lg) for section gaps.
- **Negative Space:** Use spacing to group related items rather than relying solely on borders. Margin-desktop is generous to allow the charcoal background to frame the content centered in a max-width container of 1440px.

## Elevation & Depth

In this design system, depth is achieved through **Tonal Layering** rather than traditional shadows. Shadows are kept minimal and "heavy" to maintain the industrial feel.

1. **Level 0 (Background):** `#0F1115` - The deepest layer.
2. **Level 1 (Default Surface):** `#1C1F26` - Cards, sidebars, and navigation bars.
3. **Level 2 (Elevated/Hover):** `#2D3139` - Hover states for interactive surfaces or popovers.
4. **Outlines:** Instead of ambient shadows, use 1px solid borders (`#2D3139`) to define edges. This reinforces the structured, technical look.
5. **Inner Glow:** For primary buttons, a very subtle top-edge inner highlight can be used to simulate a physical, tactile press-surface.

## Shapes

The shape language is consistently **Rounded**, striking a balance between modern software trends and industrial ergonomics.

- **Components:** Standard buttons, inputs, and cards use a `0.5rem` (8px) radius. 
- **Large Containers:** Modals and main content areas use `1rem` (16px) to soften the perimeter of the screen.
- **Feedback Elements:** Chips and badges may use a full pill-shape (`rounded-xl`) to distinguish them from actionable buttons.

## Components

- **Buttons:** 
    - *Primary:* Solid Emerald (`#10B981`) with black text (`#000000`) for maximum contrast.
    - *Secondary:* Outlined Steel Gray with white text.
    - *Ghost:* No background, Cyan text for subtle actions.
- **Input Fields:** Deep charcoal background (`#0F1115`), 1px border (`#374151`). On focus, the border transitions to Cyan with a subtle 2px outer glow.
- **Cards:** Use the Level 1 Surface color (`#1C1F26`) with a 1px border. No shadows.
- **Chips:** Small, high-contrast badges. Success chips use a dark green background with light green text; error chips use dark red background with light red text.
- **Lists:** Items are separated by subtle `1px` dividers (`#1F2937`). Hover states shift the background to `#2D3139`.
- **Data Tables:** High-density, minimal cell padding, with a frozen header using Level 2 elevation to stay distinct during scroll.