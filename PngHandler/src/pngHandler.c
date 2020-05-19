#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include <png.h>
#include <getopt.h>

#define PI 3.14159265

typedef struct PngInfo{
	int width, height;
	png_byte color_type;
    png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep* row_pointers;
} PngInfo;

typedef struct Color{
    int red;
    int green;
    int blue;
    int alpha;
} Color;

#define COUNT_COLORS 8

char* colors[] = {"red","yellow","green","blue","purple","lblue","white","black"}; 

const Color RED = {255, 0, 0, 255};
const Color YELLOW = {255, 255, 0, 255};
const Color GREEN = {0, 255, 0, 255};
const Color BLUE = {0, 0, 255, 255};
const Color PURPLE = {255, 0, 255, 255};
const Color LBLUE = {0, 255, 255, 255};
const Color WHITE = {255, 255, 255, 255};
const Color BLACK = {0, 0, 0, 255};

void printHelpMessage()
{
	FILE* fp = fopen("./help", "r");
	char c = getc(fp);
	while (c != EOF)
	{
		printf("%c", c);
		c = getc(fp);
	}
	fclose(fp);
}

void printImageInfo(PngInfo* pngInfo_ptr)
{
	printf("Информация о изображении:\n");
	printf("Ширина -- %d, высота -- %d\n", pngInfo_ptr->width, pngInfo_ptr->height);
	printf("Цветовая палитра -- ");
	switch (pngInfo_ptr->color_type)
	{
		case 0: printf("BW"); break;
		case 2: printf("RGB"); break;
		case 3: printf("кастомная"); break;
		case 4: printf("BWA"); break;
		case 6: printf("RGBA"); break;
	}
	printf("\nБит на цвет -- %d\n", pngInfo_ptr->bit_depth);
}

int checkCorrectColor(char* checkColor)
{
    int isCorrect = 0;
    for (int i = 0; i < COUNT_COLORS; ++i)
    {
        char* color = colors[i];
        if (!strcmp(color, checkColor)){
            isCorrect = 1;
            break;
        }
    }
    return isCorrect;
}

Color setColorByString(char* color)
{
    if (!strcmp("red", color)) return RED;
    if (!strcmp("yellow", color)) return YELLOW;
    if (!strcmp("green", color)) return GREEN;
    if (!strcmp("blue", color)) return BLUE;
    if (!strcmp("purple", color)) return PURPLE;
    if (!strcmp("lblue", color)) return LBLUE;
    if (!strcmp("white", color)) return WHITE;
    if (!strcmp("black", color)) return BLACK;
}

int checkCorrectCoordinates(int x, int y, PngInfo* pngInfo_ptr)
{
    if (x < 0 || x >= pngInfo_ptr->width)
        return 0;
    if (y < 0 || y >= pngInfo_ptr->height)
        return 0;
    return 1;
}

int checkDelta(int delta, PngInfo* pngInfo_ptr)
{
    if (delta == 0)
    {
        switch (pngInfo_ptr-> color_type)
        {
            case 0: printf("Обработка черно-белых изображений не поддерживается\n"); exit(1);
            case 2: delta = 3; break;
            case 3: printf("Обработка изображений с кастомными палитрами не поддерживается\n"); exit(1);
            case 4: printf("Обработка черно-белых изображений c альфа каналом не поддерживается\n"); exit(1);
            case 6: delta = 4; break;
        }
    }

    return delta;
}

void readPng(char* pngPath, PngInfo* inputPngInfo_ptr)
{
	FILE* fp = fopen(pngPath, "rb");
	if (!fp)
	{
		printf("Невозможно открыть файл\n");
		exit(1);
	}

	char signature[8];
	fread(signature, 1, 8, fp);

	if (png_sig_cmp(signature, 0, 8))
	{
		printf("Неверный формат файла\n");
		exit(1);
	}

	inputPngInfo_ptr->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	inputPngInfo_ptr->info_ptr = png_create_info_struct(inputPngInfo_ptr->png_ptr);
    png_init_io(inputPngInfo_ptr->png_ptr, fp); // Инициализация потока
    png_set_sig_bytes(inputPngInfo_ptr->png_ptr, 8); // Говорим сколько байтов сигнатуры уже считано, чтобы они не мешали при считывании инфы
    png_read_info(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr);

    inputPngInfo_ptr->width = png_get_image_width(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr);
    inputPngInfo_ptr->height = png_get_image_height(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr);
    inputPngInfo_ptr->color_type = png_get_color_type(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr);
    inputPngInfo_ptr->bit_depth = png_get_bit_depth(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr);

    png_read_update_info(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr); // Как-то обновляет указатели

    inputPngInfo_ptr->row_pointers = (png_bytep *) malloc(sizeof(png_bytep) * inputPngInfo_ptr->height);
    for (int i = 0; i < inputPngInfo_ptr->height; i++)
        inputPngInfo_ptr->row_pointers[i] = (png_byte *) malloc(png_get_rowbytes(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->info_ptr));

    png_read_image(inputPngInfo_ptr->png_ptr, inputPngInfo_ptr->row_pointers);
    fclose(fp);
}

void setPixel(int x, int y, Color color,int delta, PngInfo* pngInfo_ptr)
{
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta] = color.red;
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta +1] = color.green;
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta +2] = color.blue;
    if (delta == 4)
        (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta +3] = color.alpha;
}

void createPng(char* pngPath, PngInfo* outputPngInfo_ptr)
{
	FILE* fp = fopen(pngPath, "wb");
	if(!fp)
	{
		printf("Невозможно создать файл с таким названием");
		exit(1);
	}
	outputPngInfo_ptr-> png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); // Так и не понял разницу между read и write функциями.
	outputPngInfo_ptr-> info_ptr = png_create_info_struct(outputPngInfo_ptr-> png_ptr); // Обновляем инфо из-за нового указателя
	png_init_io(outputPngInfo_ptr-> png_ptr, fp);

	png_set_IHDR(outputPngInfo_ptr-> png_ptr, outputPngInfo_ptr-> info_ptr, outputPngInfo_ptr-> width,  // Выделение памяти для чанков IHDR
		outputPngInfo_ptr-> height, outputPngInfo_ptr-> bit_depth, outputPngInfo_ptr-> color_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_write_info(outputPngInfo_ptr-> png_ptr, outputPngInfo_ptr-> info_ptr); 
	png_write_image(outputPngInfo_ptr-> png_ptr, outputPngInfo_ptr-> row_pointers);
	png_write_end(outputPngInfo_ptr-> png_ptr, NULL);
}

int** brazenhemLine(int x0, int y0, int x1, int y1) // Возвращает массив координат точек, которые нужно закрасить
{
	// Всевозможные симметрии и замены, чтобы подогнать под рабочий случай
    int writeMod = 0; // 1 if reversed
    
    if (abs(x1-x0) < abs(y1-y0))
    {
        writeMod = 1;
        int t;
        t=x0; x0=y0; y0=t;
        t=x1; x1=y1; y1=t;
    }
    
    if (x1<=x0)
    {
        int t;
        t=x1; x1=x0; x0=t;
        t=y1; y1=y0; y0=t;
    }
    
	int deltax = abs(x1-x0);
	int deltay = abs(y1-y0);
	
	int** result = (int**) calloc(deltax+1, sizeof(int*));
	for (int i = 0; i<=deltax; i++)
	{
		result[i] = (int*) calloc(2, sizeof(int));
	}
	
	// Все из чисел с плавающей точкой домножено на deltay+1, чтобы свести вычисления к целым

	int deltaError = deltay+1;
	int error = 0;
	int y = y0;
	int diry;
	if (y1>y0){
	    diry = 1;
	} else {
	    diry = -1;
	}
	
	for (int x = x0; x<=x1; x++)
	{
	    if (writeMod == 0){
	        result[x-x0][0] = x;
		    result[x-x0][1] = y;
	    } else {
	        result[x-x0][0] = y;
		    result[x-x0][1] = x;
	    }
	    
		error += deltaError;
		if (error >= deltax+1)
		{
			y += diry;
			error -= deltax+1;
		}
	}

	return result; // Надеюсь мне никогда не придется трогать этот код
}


int** brazenhemCircle(int x0, int y0, int R) // Возвращает (x0, y0)-термированный массив координат точек, которые нужно закрасить
{
    // В отличии от линии в круге сложно узнать сколько пикселей окажутся закрашеными
    // По этому память будет взята с запасом, а символом конца последовательности будут координаты центра круга

    int x = 0, y = R;
    int d = 3 - 2 * R;
    int index = 0;

    int** result = (int**) calloc(8*R+10, sizeof(int*));
    for (int i = 0; i<8*R+10; i++)
    {
        result[i] = (int*) calloc(2, sizeof(int));
    }

    do
    {
        result[index][0] = x0 + x;
        result[index][1] = y0 + y;
        index++;
        result[index][0] = x0 + x;
        result[index][1] = y0 - y;
        index++;
        result[index][0] = x0 - x;
        result[index][1] = y0 + y;
        index++;
        result[index][0] = x0 - x;
        result[index][1] = y0 - y;
        index++;
        result[index][0] = x0 + y;
        result[index][1] = y0 + x;
        index++;
        result[index][0] = x0 + y;
        result[index][1] = y0 - x;
        index++;
        result[index][0] = x0 - y;
        result[index][1] = y0 + x;
        index++;
        result[index][0] = x0 - y;
        result[index][1] = y0 - x;
        index++;

        if (d < 0)
        {
            d = d + 4 * x + 6;
            x = x + 1;
        }
        else
        {
            if (d >= 0)
            {
                d = d + 4 * (x - y) + 10;
                x++;
                y--;
            }
        }
    } while (x <= round(R/sqrt(2.0)));
    result[index][0] = x0;
    result[index][1] = y0;
    return result;
}

int** lineEdge(int x0, int y0, int x1, int y1, int thickness)
{
    // Фигура, которая долна получиться в результате -- два круга с касающимися них двумя отрезками
    int lineMemorySize = 0;
    if (abs(x0-x1) > abs(y0-y1)){ // В си по какой-то причине нет max-а
        lineMemorySize = abs(x0-x1);
    } else {
        lineMemorySize = abs(y0-y1);
    }

    int lineEdgeMemorySize = (lineMemorySize + 8*thickness + 10)*2; // По 2 круга и 2 отрезка
    int** result = (int**) calloc(lineEdgeMemorySize, sizeof(int*));
    for (int i = 0; i<lineEdgeMemorySize; i++)
    {
        result[i] = (int*) calloc(2, sizeof(int));
    }

    int index = 0;

    int tx = 0; // Значения, на которые надо увеличить/уменьшить координаты концов отрезков, чтобы получить касательные отрезки
    int ty = 0;
    int t = thickness;

    // перед тем как подключать математику нужно убрать случаи с нулевыми угловыми коэф.
    if (y0 == y1)
    {
        ty = thickness;
    }

    if (x0 == x1)
    {
        tx = thickness;
    }

    double k = (double) (x0-x1)/(y1-y0);

    if (x0 != x1 && y0 != y1)
    {
        double preTx =  t/sqrt(1+k*k);
        tx = (int) round(preTx);
        double preTy = preTx*k;
        ty = (int) round(preTy);
    }

    int** temp = brazenhemLine(x0-tx, y0-ty, x1-tx, y1-ty);
    for (int i = 0; i<lineMemorySize; i++)
    {
        result[i+index][0] = temp[i][0];
        result[i+index][1] = temp[i][1];
    }
    index += lineMemorySize;


    temp = brazenhemLine(x0+tx, y0+ty, x1+tx, y1+ty);
    for (int i = 0; i<lineMemorySize; i++)
    {
        result[i+index][0] = temp[i][0];
        result[i+index][1] = temp[i][1];
    }
    index += lineMemorySize;

    temp = brazenhemCircle(x0, y0, thickness);
    for (int i = 0; i<(lineEdgeMemorySize - 2*lineMemorySize)/2; i++)
    {
        result[i+index][0] = temp[i][0];
        result[i+index][1] = temp[i][1];
    }
    index += (lineEdgeMemorySize - 2*lineMemorySize)/2;
    
    temp = brazenhemCircle(x1, y1, thickness);
    for (int i = 0; i<(lineEdgeMemorySize - 2*lineMemorySize)/2; i++)
    {
        result[i+index][0] = temp[i][0];
        result[i+index][1] = temp[i][1];
    }

    return result;
}

int colorMatchChecker(int x, int y, Color color, int delta, PngInfo* pngInfo_ptr)
{
    if ((pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta] != color.red)
    {
        return 0;
    }
    if ((pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta +1] != color.green)
    {
        return 0;
    }
    if ((pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta +2] != color.blue)
    {
        return 0;
    }
    if (delta == 4 && (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y-1][x*delta +3] != color.alpha)
    {
        return 0;
    }
    return 1;
}

void filler(int x, int y, Color color, int delta, PngInfo* pngInfo_ptr)
{
    if (checkCorrectCoordinates(x, y, pngInfo_ptr))
    {
        if (!colorMatchChecker(x, y, color, delta, pngInfo_ptr))
        {
            setPixel(x, y, color, delta, pngInfo_ptr);
            filler(x+1, y, color, delta, pngInfo_ptr);
            filler(x-1, y, color, delta, pngInfo_ptr);
            filler(x, y+1, color, delta, pngInfo_ptr);
            filler(x, y-1, color, delta, pngInfo_ptr);
       }
    }
}

void drawLineRaw(int x0, int y0, int x1, int y1, int thickness, char* color, PngInfo* pngInfo_ptr)
{
    int** lineEdgeCoordinates = lineEdge(x0, y0, x1, y1, thickness);
    
    int lineMemorySize = 0;
    if (abs(x0-x1) > abs(y0-y1)){
        lineMemorySize = abs(x0-x1);
    } else {
        lineMemorySize = abs(y0-y1);
    }

    int lineEdgeMemorySize = (lineMemorySize + 8*thickness + 10)*2;

    int delta = checkDelta(0, pngInfo_ptr);

    for (int i=0; i<lineEdgeMemorySize; i++) // Убрали все координаты, что вышли за рамки изображения
    {
        int x = lineEdgeCoordinates[i][0];
        int y = lineEdgeCoordinates[i][1];

        if (checkCorrectCoordinates(x, y, pngInfo_ptr))
            setPixel(x, y, setColorByString(color), delta, pngInfo_ptr);
    }

    int** fillerEdgeCoordinates = lineEdge(x0, y0, x1, y1, thickness-1);
    lineEdgeMemorySize -= 16;

    for (int i=0; i<lineEdgeMemorySize; i++) 
    {
        int x = fillerEdgeCoordinates[i][0];
        int y = fillerEdgeCoordinates[i][1];

        if (checkCorrectCoordinates(x, y, pngInfo_ptr))
            filler(x, y, setColorByString(color), delta, pngInfo_ptr);
    }

    int** fillerCoreCoordinates = brazenhemLine(x0, y0, x1, y1);

   for (int i=0; i<lineMemorySize; i++) 
    {
        int x = fillerCoreCoordinates[i][0];
        int y = fillerCoreCoordinates[i][1];

        if (checkCorrectCoordinates(x, y, pngInfo_ptr))
            filler(x, y, setColorByString(color), delta, pngInfo_ptr);
    }

}

void drawLine(int argc, char** argv, PngInfo* pngInfo_ptr)
{
    char* lineOptionsListFirstHalf_short = "x:y:";

    struct option lineOptionsListFirstHalf_long[] =
    {
        {"x0",required_argument,NULL,'x'},
        {"y0",required_argument,NULL,'y'},
        {NULL,0,NULL,0}
    };

    int x0, y0;

    for (int i = 0; i<2; i++)
    {
        int rez = getopt_long(argc,argv,lineOptionsListFirstHalf_short,lineOptionsListFirstHalf_long,NULL);
        if (optarg == NULL)
        {
            printf("Нет значения у одного из аргументов\n");
            exit(1);
        }
        switch (rez)
        {
            case 'x': sscanf(optarg, "%d", &x0); break;
            case 'y': sscanf(optarg, "%d", &y0); break;
            default : printf("Ошибка в считывании 1-го или 2-го аргумента\n"); exit(1);
        }
    }

    char* lineOptionsList_short = "x:y:c:t:";

    struct option lineOptionsList_long[] =
    {
        {"x1",required_argument,NULL,'x'},
        {"y1",required_argument,NULL,'y'},
        {"color",required_argument,NULL,'c'},
        {"thickness",required_argument,NULL,'t'},
        {NULL,0,NULL,0}
    };


    int x1, y1, thickness;
    char color[100];

    for (int i = 0; i<4; i++)
    {
        int rez = getopt_long(argc,argv,lineOptionsList_short,lineOptionsList_long,NULL);
        if (optarg == NULL)
        {
            printf("Нет значения у одного из аргументов\n");
            exit(1);
        }
        switch (rez)
        {
            case 'x': sscanf(optarg, "%d", &x1); break;
            case 'y': sscanf(optarg, "%d", &y1); break;
            case 'c': sscanf(optarg, "%s", color); break;
            case 't': sscanf(optarg, "%d", &thickness); break;
            default : printf("Ошибка в считывании 3-го или 4-го или 5-го или 6го аргумента\n"); exit(1);
        }
    }

    if (!checkCorrectColor(color)) {
        printf("Введено название неподдерживаемого цвета\n");
        exit(1);
    }

    if(!(checkCorrectCoordinates(x0, y0, pngInfo_ptr) && checkCorrectCoordinates(x1, y1, pngInfo_ptr)))
    {
        printf("Введены выходящие за рамки изображения координаты\n");
        exit(1);
    }

    if (thickness < 3 || 1000 < thickness)
    {
        printf("Недопустимая толщина линии \n");
        exit(1);
    }

    // Обработка ввода завершена

    drawLineRaw(x0, y0, x1, y1, thickness, color, pngInfo_ptr);

}

void drawCircleRaw(int x0, int y0, int R, int thickness, char* color, PngInfo* pngInfo_ptr)
{
    int delta = checkDelta(0, pngInfo_ptr);

    int** circleInnerEdge = brazenhemCircle(x0, y0, R-thickness);
    int** circleExternalEdge = brazenhemCircle(x0, y0, R+thickness);
    int** circleCore = brazenhemCircle(x0, y0, R);

    int circleInnerEdgeMemorySize = (R-thickness)*8 + 10;
    int circleExternalEdgeMemorySize = (R+thickness)*8 + 10;
    int circleCoreMemorySize = R*8 + 10;

    for(int i=0; i < circleInnerEdgeMemorySize; i++)
    {
        int x = circleInnerEdge[i][0];
        int y = circleInnerEdge[i][1];

        if (checkCorrectCoordinates(x, y, pngInfo_ptr) && (x != x0 || y != y0))
           setPixel(x, y, setColorByString(color), delta, pngInfo_ptr);
    }

    for(int i=0; i < circleExternalEdgeMemorySize; i++)
    {
        int x = circleExternalEdge[i][0];
        int y = circleExternalEdge[i][1];

        if (checkCorrectCoordinates(x, y, pngInfo_ptr) && (x != x0 || y != y0))
            setPixel(x, y, setColorByString(color), delta, pngInfo_ptr);
    }

    for(int i=0; i < circleCoreMemorySize; i++)
    {
        int x = circleCore[i][0];
        int y = circleCore[i][1];

        if (checkCorrectCoordinates(x, y, pngInfo_ptr) && (x != x0 || y != y0) && (x != 0 || y != 0))
            filler(x, y, setColorByString(color), delta, pngInfo_ptr);
    }

}

void drawPentagram(int argc, char** argv, PngInfo* pngInfo_ptr)
{
    char* pentagramOptionsList_short = "x:y:r:c:t:";

    struct option pentagramOptionsList_long[] =
    {
        {"x",required_argument,NULL,'x'},
        {"y",required_argument,NULL,'y'},
        {"radius",required_argument,NULL,'r'},
        {"color",required_argument,NULL,'c'},
        {"thickness",required_argument,NULL,'t'},
        {NULL,0,NULL,0}
    };
    
    int x,y,thickness,radius;
    char color[100];

    for (int i = 0; i<5; i++)
    {
        int rez = getopt_long(argc, argv, pentagramOptionsList_short, pentagramOptionsList_long, NULL);
        if (optarg == NULL)
        {
            printf("Нет значения у одного из аргументов\n");
            exit(1);
        }
        switch (rez)
        {
            case 'x': sscanf(optarg, "%d", &x); break;
            case 'y': sscanf(optarg, "%d", &y); break;
            case 'r': sscanf(optarg, "%d", &radius); break;
            case 'c': sscanf(optarg, "%s", color); break;
            case 't': sscanf(optarg, "%d", &thickness); break;
            default : printf("Ошибка в считывании одного из аргументов\n"); exit(1);
        }
    }

    if (!checkCorrectColor(color))
    {
        printf("Введено название неподдерживаемого цвета\n");
        exit(1);
    }

    if(!(checkCorrectCoordinates(x, y, pngInfo_ptr)))
    {
        printf("Введены выходящие за рамки изображения координаты\n");
        exit(1);
    }

    if (thickness < 3 || 1000 < thickness || thickness >= radius)
    {
        printf("Недопустимая толщина\n");
        exit(1);
    }

    drawCircleRaw(x, y, radius, thickness, color, pngInfo_ptr);

    // Мне лень делать программу, выводящую координаты n-кгольника, так что сделаем это в ручную
    int x0, y0, x1, y1, x2, y2, x3, y3, x4, y4;
    x0 = x + (int) (radius*sin(0));
    y0 = y - (int) (radius*cos(0));

    x1 = x + (int) (radius*sin(1*2*PI/5));
    y1 = y - (int) (radius*cos(1*2*PI/5));
    
    x2 = x + (int) (radius*sin(2*2*PI/5));
    y2 = y - (int) (radius*cos(2*2*PI/5));
    
    x3 = x + (int) (radius*sin(3*2*PI/5));
    y3 = y - (int) (radius*cos(3*2*PI/5));
    
    x4 = x + (int) (radius*sin(4*2*PI/5));
    y4 = y - (int) (radius*cos(4*2*PI/5));

    drawLineRaw(x0, y0, x2, y2, thickness, color, pngInfo_ptr);
    drawLineRaw(x0, y0, x2, y2, thickness, color, pngInfo_ptr);
    drawLineRaw(x2, y2, x4, y4, thickness, color, pngInfo_ptr);
    drawLineRaw(x4, y4, x1, y1, thickness, color, pngInfo_ptr);
    drawLineRaw(x1, y1, x3, y3, thickness, color, pngInfo_ptr);
    drawLineRaw(x3, y3, x0, y0, thickness, color, pngInfo_ptr);
}

void swapPixels(int x1, int y1, int x2, int y2, PngInfo* pngInfo_ptr)
{
    int delta = checkDelta(0, pngInfo_ptr);

    Color temp;

    temp.red = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta];
    temp.green = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta +1];
    temp.blue = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta +2];
    if (delta = 4)
        temp.alpha = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta +3];

    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta] = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta];
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta +1] = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta +1];
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta +2] = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta +2];
    if (delta == 4)
        (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y1-1][x1*delta +3] = (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta +3];

    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta] = temp.red;
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta +1] = temp.green;
    (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta +2] = temp.blue;
    if (delta = 4)
        (pngInfo_ptr->row_pointers)[pngInfo_ptr->height -y2-1][x2*delta +3] = temp.alpha;
}

void reflection(int argc, char** argv, PngInfo* pngInfo_ptr)
{
    char* reflectionOptionsListFirstHalf_short = "x:y:";

    struct option reflectionOptionsListFirstHalf_long[] =
    {
        {"x0",required_argument,NULL,'x'},
        {"y0",required_argument,NULL,'y'},
        {NULL,0,NULL,0}
    };

    int x0, y0;

    for (int i = 0; i<2; i++)
    {
        int rez = getopt_long(argc,argv,reflectionOptionsListFirstHalf_short,reflectionOptionsListFirstHalf_long,NULL);
        if (optarg == NULL)
        {
            printf("Нет значения у одного из аргументов\n");
            exit(1);
        }
        switch (rez)
        {
            case 'x': sscanf(optarg, "%d", &x0); break;
            case 'y': sscanf(optarg, "%d", &y0); break;
            default : printf("Ошибка в считывании 1-го или 2-го аргумента\n"); exit(1);
        }
    }

    int x1, y1;
    char axis;

    char* reflectionOptionsListSecondHalf_short = "x:y:a:";

    struct option reflectionOptionsListSecondHalf_long[] =
    {
        {"x1",required_argument,NULL,'x'},
        {"y1",required_argument,NULL,'y'},
        {"axis",required_argument,NULL,'a'},
        {NULL,0,NULL,0}
    };

    for (int i = 0; i<3; i++)
    {
        int rez = getopt_long(argc,argv,reflectionOptionsListSecondHalf_short,reflectionOptionsListSecondHalf_long,NULL);
        if (optarg == NULL)
        {
            printf("Нет значения у одного из аргументов\n");
            exit(1);
        }
        switch (rez)
        {
            case 'x': sscanf(optarg, "%d", &x1); break;
            case 'y': sscanf(optarg, "%d", &y1); break;
            case 'a': sscanf(optarg, "%c", &axis); break;
            default : printf("Ошибка в считывании 3-го или 4-го или 5-го аргумента\n"); exit(1);
        }
    }

    if(!(checkCorrectCoordinates(x0, y0, pngInfo_ptr) && checkCorrectCoordinates(x1, y1, pngInfo_ptr)))
    {
        printf("Введены выходящие за рамки изображения координаты\n");
        exit(1);
    }

    if (x1 <= x0 || y1 <= y0)
    {
        printf("Введены координаты невозможные для построения прямоугольника\n");
        exit(1);
    }

    if (!(axis == 'x' || axis == 'y'))
    {
        printf("Введено неподдерживаемое значение axis\n");
        exit(1);
    }

    // Обработка ввода завершена

    if (axis == 'x')
    {
        for (int i = x0; i <= x1/2; i++)
        {
            for (int j = y0; j < y1; j++)
                swapPixels(i, j, x1-i, j, pngInfo_ptr);
        }
    }

    if (axis == 'y')
    {
        for (int i = x0; i <= x1; i++)
        {
            for (int j = y0; j < y1/2; j++)
                swapPixels(i, j, i, y1-j, pngInfo_ptr);
        }
    }

}


int main(int argc, char** argv)
{
	opterr = 0; // Запрет на вывод ошибок для getopt
    
// Проверка случая без аргументов
	if (argc == 1)
	{
		printf("Print --help for help\n");
		return 0;
	}

	PngInfo pngInfo;

// Проверка случая с флагом help

	if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0){
		printHelpMessage();
		return 0;
	} else {
		readPng(argv[1], &pngInfo);
	}

	const char* basicOptionsList_short = "ilpr";

	const struct option basicOptionsList_long[] =
	{
        {"info",no_argument,NULL,'i'},
        {"line",no_argument,NULL,'l'},
        {"pentagram",no_argument,NULL,'p'},
        {"reflection",no_argument,NULL,'r'},
        {NULL,0,NULL,0}
    };

    int basicFlag = getopt_long(argc, argv, basicOptionsList_short, basicOptionsList_long, NULL);

    if (basicFlag != -1)
    {
    	switch(basicFlag)
    	{
    		case 'i': printImageInfo(&pngInfo); return 1;
    		case 'l': drawLine(argc, argv, &pngInfo); break;
    		case 'p': drawPentagram(argc, argv, &pngInfo); break;
    		case 'r': reflection(argc, argv, &pngInfo); break;  
    		default : printf("Неправильный флаг обработки изображения\n"); return 1;
    	}
    } else {
    	printf("Не выбран вариант обработки изображения\n");
    	return 1;
    }

    const char* outputOptionsList_short = "o:";

    const struct option outputOptionsList_long[] =
    {
        {"output",required_argument,NULL,'o'},
        {NULL,0,NULL,0}
    };

    int outFlag = getopt_long(argc, argv, outputOptionsList_short, outputOptionsList_long, NULL);

    char* outputPath = calloc(100, sizeof(char));

    if (outFlag != 'o') {
        printf("Неправильный/отсутствует флаг вывода, поставлено значение по умолчанию\n");
        outputPath = "./out.png";
    } else {
        sscanf(optarg, "%s", outputPath);
    }

    createPng(outputPath, &pngInfo);
    printf("Программа завершила свою работу успешно.\n");
	return 0;
}
