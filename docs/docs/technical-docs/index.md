---
pageType: home

hero:
  name: Safi Engine
  text: Technical Documentation
  tagline: Internal reference for the C++/Vulkan rendering backend
  actions:
    - theme: brand
      text: Architecture Overview
      link: /overview
    - theme: alt
      text: GitHub
      link: https://github.com/safi-io/Safi-ECS-Game-Engine
  image:
    src: /rspress-icon.png
    alt: Safi Engine
features:
  - title: Vulkan Rendering
    details: Dual-pipeline architecture — 3D scene with Blinn-Phong shading plus a 2D UI overlay with alpha blending.
    icon: "\u{1F5A5}"
  - title: C# ECS + C++ Backend
    details: Game logic runs in C# (Mono) via an ECS. The native Vulkan renderer communicates through a P/Invoke bridge.
    icon: "\u{1F517}"
  - title: glTF Mesh Loading
    details: Load meshes from glTF/GLB files with base color textures, or generate procedural primitives (box, sphere, capsule, etc.).
    icon: "\u{1F4E6}"
  - title: Dynamic Lighting
    details: Up to 8 real-time lights — directional, point, and spot — with per-fragment Blinn-Phong evaluation.
    icon: "\u{1F4A1}"
---
