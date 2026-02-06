/** @type {import('next').NextConfig} */
const nextConfig = {
  typescript: {
    // Mengizinkan build selesai meskipun ada error TypeScript
    ignoreBuildErrors: true,
  },
};

export default nextConfig;