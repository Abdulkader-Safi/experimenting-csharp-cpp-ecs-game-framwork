import * as path from 'node:path';
import { defineConfig } from '@rspress/core';

export default defineConfig({
  root: path.join(__dirname, 'docs'),
  title: 'Safi Game Engine',
  themeConfig: {
    socialLinks: [
      {
        icon: 'github',
        mode: 'link',
        content: 'https://github.com/Abdulkader-Safi/Safi-ECS-Game-Engine',
      },
    ],
  },
});
