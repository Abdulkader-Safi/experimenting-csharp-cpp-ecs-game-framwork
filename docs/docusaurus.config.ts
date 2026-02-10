import { themes as prismThemes } from "prism-react-renderer";
import type { Config } from "@docusaurus/types";
import type * as Preset from "@docusaurus/preset-classic";

const config: Config = {
  title: "Safi Game Engine",
  tagline: "A Bevy-inspired ECS engine with C# logic and C++ Vulkan rendering",
  favicon: "img/favicon.ico",

  future: {
    v4: true,
  },

  url: "https://your-docusaurus-site.example.com",
  baseUrl: "/",

  onBrokenLinks: "throw",

  i18n: {
    defaultLocale: "en",
    locales: ["en"],
  },

  plugins: ["docusaurus-plugin-auto-sidebars", "docusaurus-lunr-search"],

  presets: [
    [
      "classic",
      {
        docs: {
          sidebarPath: "./sidebars.ts",
        },
        blog: {
          showReadingTime: true,
          feedOptions: {
            type: ["rss", "atom"],
            xslt: true,
          },
          onInlineTags: "warn",
          onInlineAuthors: "warn",
          onUntruncatedBlogPosts: "warn",
        },
        theme: {
          customCss: "./src/css/custom.css",
        },
      } satisfies Preset.Options,
    ],
  ],

  themeConfig: {
    colorMode: {
      respectPrefersColorScheme: true,
    },
    navbar: {
      title: "Safe Game Engine",
      items: [
        {
          type: "docSidebar",
          sidebarId: "tutorialSidebar",
          position: "left",
          label: "Docs",
        },
        { to: "/blog", label: "Blog", position: "left" },
      ],
    },
    footer: {
      style: "dark",
      links: [
        {
          title: "Docs",
          items: [
            {
              label: "Getting Started",
              to: "/docs/intro",
            },
            {
              label: "ECS Guide",
              to: "/docs/ecs/world",
            },
          ],
        },
      ],
      copyright: `Copyright \u00a9 ${new Date().getFullYear()} C++/C# Game Engine. Built with Docusaurus.`,
    },
    prism: {
      theme: prismThemes.github,
      darkTheme: prismThemes.dracula,
      additionalLanguages: ["csharp", "bash", "makefile"],
    },
  } satisfies Preset.ThemeConfig,
};

export default config;
