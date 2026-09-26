/** @type {import('tailwindcss').Config} */
module.exports = {
  content: [
    './src/pages/**/*.{js,ts,jsx,tsx,mdx}',
    './src/components/**/*.{js,ts,jsx,tsx,mdx}',
    './src/app/**/*.{js,ts,jsx,tsx,mdx}',
  ],
  theme: {
    extend: {
      colors: {
        navy: {
          900: '#0b132b',
          800: '#1c2541',
          700: '#3a506b',
        },
        cyan: {
          400: '#38bdf8',
          500: '#0284c7',
        }
      }
    },
  },
  plugins: [],
}
