#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <windows.h>

#pragma pack(push, 1)

struct BMPHeader 
{
	uint16_t bfType;      // "BM"
	uint32_t bfSize;      // Размер файла
	
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	
	uint32_t bfOffBits;   // Смещение к данным изображения
};

struct DIBHeader 
{
	uint32_t biSize;          // Размер этого заголовка (40 байт)
	
	int32_t biWidth;          // Ширина
	int32_t biHeight;         // Высота
	
	uint16_t biPlanes;        // Количество плоскостей (должно быть 1)
	uint16_t biBitCount;      // Биты на пиксель (24 или 32)
	uint32_t biCompression;   // Сжатие (0 - без сжатия)
	uint32_t biSizeImage;     // Размер изображения
	
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};

#pragma pack(pop)

struct Pixel 
{
	uint8_t r, g, b, a;
};

class BMPImage 
{
public:
	
	int width;
	int height;
	
	std::vector<Pixel> pixels;
	
	bool load(const std::string& filename);
	bool save(const std::string& filename);
	
	void drawX(int x, int y);
	void printToConsole() const;

	bool isValidColor(const Pixel& p) const;
};

bool BMPImage::load(const std::string& filename) 
{
	std::ifstream in(filename, std::ios::binary);
	
	if (!in.is_open()) 
	{
		std::cerr << "Failed to open file."<<std::endl;
		return false;
	}
	
	BMPHeader bmpHeader;
	in.read(reinterpret_cast<char*>(&bmpHeader), sizeof(bmpHeader));
	
	// валидация файл BMP
	if (bmpHeader.bfType != 0x4D42) 
	{ 
		std::cerr << "Not a BMP file."<<std::endl;
		return false;
	}
	
	DIBHeader dibHeader;
	in.read(reinterpret_cast<char*>(&dibHeader), sizeof(dibHeader));
	
	if (dibHeader.biBitCount != 24 && dibHeader.biBitCount != 32) 
	{
		std::cerr << "Only 24 or 32-bit BMP supported."<<std::endl;
		return false;
	}
	
	width = dibHeader.biWidth;
	height = dibHeader.biHeight;
	
	in.seekg(bmpHeader.bfOffBits, std::ios::beg);
	
	size_t rowSize = ((dibHeader.biBitCount * width + 31) / 32) * 4;
	
	pixels.resize(width * height);
	
	for (int y = 0; y < height; ++y) 
	{
		size_t rowPos = y * rowSize;
		
		in.seekg(bmpHeader.bfOffBits + rowPos, std::ios::beg);
		
		for (int x = 0; x < width; ++x) 
		{
			Pixel p;
			
			in.read(reinterpret_cast<char*> (&p.b), 1);
			in.read(reinterpret_cast<char*> (&p.g), 1);
			in.read(reinterpret_cast<char*> (&p.r), 1);
			
			if (dibHeader.biBitCount == 32) 
			{
				in.read(reinterpret_cast<char*>(&p.a),1);
			} else 
			{
				p.a = 255;
			}
			
			// BMP хранит снизу вверх
			pixels[(height - y - 1) * width + x] = p; 
		}
		
		// пропускаем паддинги
		size_t padding = rowSize - ((dibHeader.biBitCount / 8) * width);
		in.seekg(padding, std::ios::cur);
	}
	
	return true;
}

bool BMPImage::save(const std::string& filename) 
{
	std::ofstream out(filename, std::ios::binary);
	
	BMPHeader bmpHeader{};
	DIBHeader dibHeader{};
	
	// сохраняем как 24 бита для простоты
	int rowSize = ((24 * width + 31) / 32) * 4;
	
	size_t pixelDataSize = rowSize * height;
	
	bmpHeader.bfType = 0x4D42; // 'BM'
	bmpHeader.bfOffBits = sizeof(BMPHeader) + sizeof(DIBHeader);
	bmpHeader.bfSize = bmpHeader.bfOffBits + pixelDataSize;
	
	dibHeader.biSize = sizeof(DIBHeader);
	dibHeader.biWidth = width;
	dibHeader.biHeight = height;
	dibHeader.biPlanes = 1;
	
// Для сохранения как 24 бита
#ifdef _WIN64
#pragma message("Ensure to compile with support for packing")
#endif
	
	// установка biBitCount=24:
	dibHeader.biBitCount = 24;
	
	// без сжатия:
	dibHeader.biCompression = 0;
	
	// размер изображения:
	dibHeader.biSizeImage = pixelDataSize;
	
	out.write(reinterpret_cast<const char*>(&bmpHeader), sizeof(bmpHeader));
	out.write(reinterpret_cast<const char*>(&dibHeader), sizeof(dibHeader));
	
	// снизу вверх
	for (int y = height-1; y >= 0; --y) 
	{
		size_t rowStartPos = y*width;
		
		for (int x=0; x<width; ++x) 
		{
			Pixel& p = pixels[rowStartPos + x];
			out.put(p.b);
			out.put(p.g);
			out.put(p.r);
		}
		
		// паддинги:
		size_t paddingSize = rowSize - 3 * width;
		
		for(size_t i = 0; i < paddingSize; ++i)
			out.put(0);
	}
	
	return true;
}

void BMPImage::printToConsole() const 
{
	for (int y=0; y < height; ++y)
	{
		for(int x=0; x < width; ++x)
		{
			const Pixel& p = pixels[y * width + x];
			
			// Черный цвет — символ "#"
			if(p.r==0 && p.g==0 && p.b==0)
			{
				std::cout<<"#"; 
			}
			else if(p.r==255 && p.g==255 && p.b==255)
			{
				std::cout<<" "; // Белый цвет — символ "пробел"
			}
			else
			{
				std::cout<<"?"; // Другие цвета — символ "вопросительный знак"
			}
		}
		
		std::cout<<std::endl;
	}
}

bool BMPImage::isValidColor(const Pixel& p) const
{
	return ((p.r==0 && p.g==0 && p.b==0) || (p.r==255 && p.g==255 && p.b==255));
}

void BMPImage::drawX(int x, int y)
{
	auto plotLine = [this](int x1, int y1, int x2, int y2)
	{
		
		int dx = abs(x2-x1);
		int dy = abs(y2-y1);
		
		int err = dx-dy;
		
		int sx = x1<x2?1:-1;
		int sy = y1<y2?1:-1;
		
		while(true)
		{
			// рисуем белым
			if((x1 >= 0 && x1 < width) && (y1 >= 0 && y1 < height))
				pixels[y1*width+x1] = {255,255,255};
			
			if(x1 == x2 && y1 == y2)
				break;
			
			int e2 = 2 * err;
			
			if(e2 > -dy) 
			{
				err -= dy;
				x1 += sx;
			}
			
			if(e2 < dx) 
			{
				err += dx;
				y1 += sy;
			}
		};
	};
	
	// рисуем первую диагональ (/)
	plotLine(x, y, x+10, y+10);
	
	// рисуем вторую диагональ (\)
	plotLine(x, y+10, x+10, y);
	
}

int main() 
{
	BMPImage img;
	
	std::string filename_in, filename_out;
	
	std::cout<<"Enter input BMP file name: ";
	std::getline(std::cin, filename_in);
	
	if(!img.load(filename_in))
	{
		std::cerr<<"Failed to load image."<<std::endl;
		return 1;
	}
	
	// проверка что все цвета — черные или белые
	for(const auto& p:img.pixels)
	{
		if(!img.isValidColor(p))
		{
			std::cerr<<"Image contains colors other than black and white."<<std::endl;
			return 1;
		}
	}
	
	// вывод изображения в консоль
	img.printToConsole();
	
	// рисуем крест на изображении по центру или по координатам
	int centerX = img.width/2 - 5; 
	int centerY = img.height/2 - 5;
	
	// рисуем крест: две линии по диагонали
	img.drawX(centerX, centerY);
	
	std::cout<<std::endl;
	std::cout<<"After drawing X:";
	std::cout<<std::endl;
	
	// выводим обновленное изображение в консоль
	img.printToConsole();
	
	// запрос имени файла для сохранения результата
	std::cout<<"Enter output BMP file name: ";
	std::getline(std::cin, filename_out);
	
	if(!img.save(filename_out))
	{
		std::cerr<<"Failed to save image."<<std::endl;
		return 1;
	}
	
	return 0;
	
}
