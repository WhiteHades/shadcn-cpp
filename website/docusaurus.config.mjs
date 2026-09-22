import {fileURLToPath} from 'node:url';
import path from 'node:path';
const here = path.dirname(fileURLToPath(import.meta.url));

export default {
  title: 'shadcn-cpp',
  tagline: 'Native C++23 and Qt 6 UI components inspired by shadcn/ui.',
  url: 'https://whitehades.github.io',
  baseUrl: '/shadcn-cpp/',
  organizationName: 'WhiteHades',
  projectName: 'shadcn-cpp',
  trailingSlash: false,
  onBrokenLinks: 'throw',
  markdown: {hooks: {onBrokenMarkdownLinks: 'throw'}},
  i18n: {defaultLocale: 'en', locales: ['en']},
  presets: [
    ['classic', {
      docs: {
        path: path.resolve(here, '../docs'),
        routeBasePath: '/',
        sidebarPath: path.resolve(here, 'sidebars.mjs'),
      },
      blog: false,
      theme: {customCss: path.resolve(here, 'src/css/custom.css')},
    }],
  ],
  themeConfig: {
    colorMode: {defaultMode: 'light', respectPrefersColorScheme: true},
    navbar: {
      title: 'shadcn-cpp',
      items: [
        {type: 'docSidebar', sidebarId: 'guide', position: 'left', label: 'Docs'},
        {to: '/api', label: 'C++ API', position: 'left'},
        {href: 'https://github.com/WhiteHades/shadcn-cpp', label: 'GitHub', position: 'right'},
      ],
    },
    footer: {
      style: 'light',
      copyright: 'shadcn-cpp 0.1.0 · MIT · Independent port, under development.',
    },
    prism: {additionalLanguages: ['cpp', 'cmake', 'bash', 'json']},
  },
};
