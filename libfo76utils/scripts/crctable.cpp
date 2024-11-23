
#include <cstdio>
#include <cstdlib>
#include <cstdint>

int main(int argc, char **argv)
{
  for (int i = 1; i < argc; i++)
  {
    char    *endp = (char *) 0;
    std::uint32_t polynomial = std::uint32_t(std::strtol(argv[i], &endp, 0));
    std::printf("const std::uint32_t crc32Table_%08X[256] =\n",
                (unsigned int) polynomial);
    std::printf("{\n");
    for (size_t j = 0; j < 256; j++)
    {
      unsigned int  crcValue = (unsigned int) j;
      for (size_t k = 0; k < 8; k++)
        crcValue = (crcValue >> 1) ^ (polynomial & (0U - (crcValue & 1U)));
      if ((j % 6) == 0)
        std::fputc(' ', stdout);
      std::printf(" 0x%08X", crcValue);
      if (j == 255)
        std::fputc('\n', stdout);
      else if ((j % 6) != 5)
        std::fputc(',', stdout);
      else
        std::printf(",\n");
    }
    std::printf("};\n\n");
  }
  return 0;
}

