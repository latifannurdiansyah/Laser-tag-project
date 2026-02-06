/** @type {import('next').NextConfig} */
const nextConfig = {
  typescript: {
    // Mengizinkan build selesai meskipun ada error TypeScript
    ignoreBuildErrors: true,
  },
  eslint: {
    // Mengizinkan build selesai meskipun ada warning/error ESLint
    ignoreDuringBuilds: true,
  },
};

export default nextConfig;