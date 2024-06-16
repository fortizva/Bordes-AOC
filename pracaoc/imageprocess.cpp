/***************************************************************************
 *   Copyright (C) 2023 by pilar                                           *
 *   pilarb@unex.es                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "imageprocess.h"


void imageprocess::suavizado(unsigned char * imagenO, unsigned char * imagenD, int W, int H, int * kernel, int N)
{
    // %0 = imagenO; %1 = imagenD; %2 = W; %3 = H; %4 = kernel; %5 = N

    asm volatile(

        "mov %0, %%rsi;" // imagenO
        "mov %1, %%rdi;" // imagenD
        "mov $0, %%r8d;" //y
    "bSuavizadoY:"
        "mov $0, %%r9d;" //x
        "bSuavizadoX:"
            "mov %4, %%rbx;" //kernel
            "mov $0, %%eax;" //acum
            "mov $0, %%ecx;" //totalK
            "mov %5, %%r10d;"
            "neg %%r10d;" //j
            "bSuavizadoKernelJ:"
                "mov %5, %%r11d;"
                "neg %%r11d;" //i
                "bSuavizadoKernelI:"
                    "mov %%r8d, %%r12d;"
                    "add %%r10d, %%r12d;" //y+j
                    "mov %%r9d, %%r13d;"
                    "add %%r11d, %%r13d;" //x+i
                    "cmp $0, %%r12d;"
                    "jl sigKernelI;"
                    "cmp %3, %%r12d;"
                    "jge sigKernelI;"
                    "cmp $0, %%r13d;"
                    "jl sigKernelI;"
                    "cmp %2, %%r13d;"
                    "jge sigKernelI;"

                    "mov %2, %%r14d;"
                    "imul %%r12d, %%r14d;"
                    "add %%r13d, %%r14d;"
                    "movsx %%r14d, %%r14;"
                    "movzbl (%%rsi, %%r14), %%edx;"

                    "imull (%%rbx), %%edx;"
                    "add %%edx, %%eax;"
                    "add (%%rbx), %%ecx;"

                "sigKernelI:"
                    "add $4, %%rbx;"
                    "inc %%r11d;"
                    "cmp %5, %%r11d;"
                    "jle bSuavizadoKernelI;"
            "sigKernelJ:"
                "inc %%r10d;"
                "cmp %5, %%r10d;"
                "jle bSuavizadoKernelJ;"
            "mov $0, %%edx;"
            "div %%ecx;" // acum/kernel_t
            "mov %%r8d, %%r14d;"
            "imul %2, %%r14d;"
            "add %%r9d, %%r14d;"
            "movsx %%r14d, %%r14;"
            "mov %%al, (%%rdi, %%r14);"
            "inc %%r9d;"
            "cmp %2, %%r9d;"
            "jl bSuavizadoX;"
        "inc %%r8d;"
        "cmp %3, %%r8d;"
        "jl bSuavizadoY;"


        :
        : "m" (imagenO), "m" (imagenD), "m" (W), "m" (H), "m" (kernel), "m" (N)
        : "%rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11", "%r12",
          "%r13", "%r14", "memory"

    );


}

void imageprocess::umbralizar(unsigned char * imagenO, unsigned char * imagenD, unsigned char umbral, int W, int H)
{
    // %0 = imagenO; %1 = imagenD; %2 = umbral; %3 = W; %4 = H

    asm volatile(
        /*
            dirImagenO = imagenO;
            dirImagenD = imagenD;
            Para(y=0; y<H; y++)
            {
                Para(x=0; x<W; x++)
                {
                    offsetPixel = y*W + x;
                    Si([dirImagenO + offsetPixel] <= umbral)
                        [dirImagenD + offsetPixel] = 0;
                    Sino
                        [dirImagenD + offsetPixel] = 255;
                }
            }
        */
            "mov %0, %%rsi;" // imagenD
            "mov %1, %%rdi;" // imagenO
            "mov $0, %%ecx;" // Contador
             // Bucle y
            "umbralizarYLoop:"
                "mov $0, %%ebx;" // Contador
                // Bucle x
                "umbralizarXLoop:"
                    // Calcular offsetPixel
                    "movl %3, %%eax;" // AX = W
                    "mul %%ecx;"  // EAX = W * y
                    "add %%rbx, %%rax;" // RAX = (W*y) + x
                    "mov %%rax, %%r8;" // offsetPixel

                    "movb %2, %%r9b;"
                    "cmpb (%%r8, %%rsi), %%r9b;"
                    "ja ceilUmbralizarIf;"
                        "movb $0, (%%r8, %%rdi);"
                        "jmp umbralizarEndIf;"
                    "ceilUmbralizarIf:"
                        "movb $255, (%%r8, %%rdi);"
                    "umbralizarEndIf:"
                    // Fin bucle x
                    "inc %%ebx;"
                    "cmp %3, %%ebx;"
                    "jb umbralizarXLoop;"

                // Fin bucle y
                "inc %%ecx;"
                "cmp %4, %%ecx;"
                "jb umbralizarYLoop;"

        :
        : "m" (imagenO), "m" (imagenD), "m" (umbral), "m" (W), "m" (H)
        : "%rsi","%rdi","%rdx", "%ecx", "%rbx", "%rax", "%r8", "%r9", "memory"

    );

}

void imageprocess::calculoGradientes(unsigned char* imagen, Gradient* gradientes, int W, int H)
{
    int kernel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int * sobel_x = (int *) &kernel_x;
    int kernel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    int *  sobel_y = (int *) &kernel_y;

    // %0 = imagen; %1 = gradientes; %2 = W; %3 = H; %4 = sobel_x; %5 = sobel_y

    asm volatile(
        /*
            dirImagenO = imagenO;
            dirGradientes = gradientes;
            Para(y=0; y<H; y++)
            {
                Para(x=0; x<W; x++)
                {
                    dirSobelX = sobel_x;
                    dirSobelY = sobel_y;
                    gradX = 0;
                    gradY = 0
                    Para(j=-1; j<=1; j++)
                    {
                        Para(i=-1; i<=1; i++)
                        {
                            Si((x+i)>=0 y (x+i)<W y (y+j)>=0 y (y+j)<H)
                            {
                                offsetPixel = (y + j) * W + (x + i);
                                gradX += [dirImagenO + offsetPixel]*[dirSobelX];
                                gradY += [dirImagenO + offsetPixel]*[dirSobelY];
                            }
                                dirSobelX += 4;
                                dirSobelY += 4;
                        }
                    }
                    offsetPixel = y*W + x;
                    [dirGradientes + offsetPixel*8].dx = gradX;
                    [dirGradientes + offsetPixel*8].dy = gradY;
                }
            }
        */

                "mov %0, %%rsi;"
                "mov %1, %%rdi;"

                "mov $0, %%ebx;"
                "calculoGradientesYLoop:"
                    "mov $0, %%ecx;"
                    "calculoGradientesXLoop:"
                        "mov %4, %%r8;" // sobel_x
                        "mov %5, %%r9;" // sobel_y
                        "mov $0, %%r10;" // gradX
                        "mov $0, %%r11;" // gradY

                        "mov $-1, %%r12d;" // Loop j
                        "calculoGradientesJLoop:"
                            "mov $-1, %%r13d;"
                            "calculoGradientesILoop:" // Loop i
                            // Si((x+i)>=0 y (x+i)<W y (y+j)>=0 y (y+j)<H)
                                "mov $0, %%r14;"
                                "mov %%ecx, %%r14d;"
                                "add %%r13d, %%r14d;"
                                "cmp $0, %%r14d;" // ! (x+i) >= 0
                                "jb calculoGradientesILoopFin;"

                                "cmp %2, %%r14d;" // ! (x+i) < W
                                "jae calculoGradientesILoopFin;"

                                "mov $0, %%rax;"
                                "mov %%ebx, %%eax;"
                                "add %%r12d, %%eax;"
                                "cmp $0, %%eax;" // ! (y+j) >= 0
                                "jb calculoGradientesILoopFin;"

                                "cmp %2, %%eax;" // ! (y+j) < H
                                "jae calculoGradientesILoopFin;"

                                // If content
                                "mov $0, %%rdx;"
                                "mulw %2;" // (y + j) * W
                                /*"shl $16, %%rdx;" // Move DX to upper registry on RDX
                                "add %%rdx, %%rax;" // Move DX:AX to RAX */
                                "add %%eax, %%r14d;" // offsetPixel

                                // Calculate gradX and gradY
                                "mov $0, %%rax;"
                                "movb (%%r14, %%rsi), %%al;"
                                "mulw (%%r8);"
                                "add %%eax, %%r10d;"

                                "mov $0, %%rax;"
                                "movb (%%r14, %%rsi), %%al;"
                                "mulw (%%r9);"
                                "add %%eax, %%r11d;"

                                "calculoGradientesILoopFin:"
                                "add $4, %%r8;"
                                "add $4, %%r9;"

                                "inc %%r13d;"
                                "cmp $1, %%r13d;"
                                "jle calculoGradientesILoop;"

                            "inc %%r12d;"
                            "cmp $1, %%r12d;"
                            "jle calculoGradientesJLoop;"

                        "mov %2, %%eax;"
                        //"mov $0, %%rdx;"
                        "mul %%ebx;"
                        //"shl $16, %%rdx;"
                        //"add %%rdx, %%rax;"
                        "mov $0, %%r14;"
                        "mov %%ecx, %%r14d;"
                        "add %%eax, %%r14d;" // offsetPixel = y*W + x

                        "mov %%r10b, (%%rdi, %%r14, 8);"
                        "mov %%r11b, 4(%%rdi, %%r14, 8);"

                        "inc %%ecx;"
                        "cmp %2, %%ecx;"
                        "jb calculoGradientesXLoop;"

                "inc %%ebx;"
                "cmp %3, %%ebx;"
                "jb calculoGradientesYLoop;"

        ";"

        :
        : "m" (imagen), "m" (gradientes), "m" (W), "m" (H), "m" (sobel_x), "m" (sobel_y)
        : "rax", "%rbx", "%rcx", "%rdx", "%rsi", "%rdi", "%r8", "%r9", "%r10", "%r11", "%r12", "%r13", "%r14", "memory"

    );

}



void imageprocess::supresionNoMaximo(int * gradNorm, unsigned char * gradDir, unsigned char* imagenD, int W, int H)
{

    // %0 = gradNorm; %1 = gradDir; %2 = imagenD; %3 = W; %4 = H

    asm volatile(
        /*
            dirGradNorm = gradNorm;
            dirGradDir = gradDir;
            dirImagenD = imagenD;
            Para(y=1; y<H-1; y++)
            {
                Para(x=1; x<W-1; x++)
                {
                    offsetPixel = y*W + x;
                    Si([dirGradNorm + offsetPixel*4]<=255)
                        [dirImagenD+offsetPixel]=[dirGradNorm+offsetPixel*4];
                    Sino
                        [dirImagenD+offsetPixel]=255;
                    Si([dirGradDir+offsetPixel]==0)
                    {
                        vOffset1 = offsetPixel-1;
                        vOffset2 = offsetPixel+1;
                    }
                    Sino Si([dirGradDir+offsetPixel]==1)
                    {
                        vOffset1 = offsetPixel-W-1;
                        vOffset2 = offsetPixel+W+1;
                    }
                    Sino Si([dirGradDir+offsetPixel]==2)
                    {
                        vOffset1 = offsetPixel-W;
                        vOffset2 = offsetPixel+W;
                    }
                    Sino Si([dirGradDir+offsetPixel]==3)
                    {
                        vOffset1 = offsetPixel-W+1;
                        vOffset2 = offsetPixel+W-1;
                    }
                    Si([dirGradNorm+offsetPixel*4]<=[dirGradNorm+vOffset1*4] o [dirGradNorm+offsetPixel*4]<[dirGradNorm+vOffset2*4])
                        [dirImagenD+offsetPixel]=0;
                }
            }
        */

        ";"

        :
        : "m" (gradNorm), "m" (gradDir), "m" (imagenD), "m" (W), "m" (H)
        : "memory"
    );

}

void imageprocess::dobleUmbralizacion(unsigned char* imagenO, unsigned char* imagenD, unsigned char umbralMin, unsigned char umbralMax, int W, int H)
{

    // %0 = imagenO; %1 = imagenD; %2 = umbralMin; %3 = umbralMax; %4 = W; %5 = H

    asm volatile(
        /*
        dirImagenO = imagenO;
        dirImagenD = imagenD;
        Para(y=1; y<H-1; y++)
        {
            Para(x=1; x<W-1; x++)
            {
                offsetPixel = y*W + x;
                Si([dirImagenO+offsetPixel] >= umbralMax)
                {
                    [dirImagenD+offsetPixel] = 255;
                }
                Sino
                {
                    esBorde = Falso;
                    Si([dirImagenO+offsetPixel] >= umbralMin)
                    {
                        Para(j=-1; j<=1 y no esBorde; j++)
                        {
                            Para(i=-1; i<=1 y no esBorde; i++)
                            {
                                vOffset = (y+j)*W + (x+i);
                                Si([dirImagenO+vOffset] >= umbralMax o [dirImagenD+vOffset] == 255)
                                {
                                    esBorde = Cierto;
                                }
                            }
                        }
                    }
                    Si(esBorde)
                        [dirImagenD+offsetPixel] = 255;
                    Sino
                        [dirImagenD+offsetPixel] = 0;
                }
            }
        }
        */
        ";"

        :
        : "m" (imagenO), "m" (imagenD), "m" (umbralMin), "m" (umbralMax), "m" (W), "m" (H)
        : "memory"

    );

}


int imageprocess::detectaContorno(unsigned char * imagen, unsigned char * visitados, int ini_x, int ini_y, Point direcciones[8], Point contorno[5000], int W, int H)
{
    int nPuntos = 0;

    // %0 = nPuntos; %1 = imagen; %2 = visitados; %3 = ini_x; %4 = ini_y; %5 = direcciones; %6 = contorno; %7 = W; %8 = H

    asm volatile(
        /*
        dirImagenO = imagenO;
        dirImagenD = imagenD;
        Para(y=1; y<H-1; y++)
        {
            Para(x=1; x<W-1; x++)
            {
                offsetPixel = y*W + x;
                Si([dirImagenO+offsetPixel] >= umbralMax)
                {
                    [dirImagenD+offsetPixel] = 255;
                }
                Sino
                {
                    esBorde = Falso;
                    Si([dirImagenO+offsetPixel] >= umbralMin)
                    {
                    Para(j=-1; j<=1 y no esBorde; j++)
                    {
                        Para(i=-1; i<=1 y no esBorde; i++)
                        {
                            vOffset = (y+j)*W + (x+i);
                            Si([dirImagenO+vOffset] >= umbralMax o [dirImagenD+vOffset] == 255)
                            {
                                esBorde = Cierto;
                            }
                        }
                    }
                }
                Si(esBorde)
                [dirImagenD+offsetPixel] = 255;
                Sino
                [dirImagenD+offsetPixel] = 0;
                }
            }
        }
        */
        ";"

        : "=m" (nPuntos)
        : "m" (imagen), "m" (visitados), "m" (ini_x), "m" (ini_y), "m" (direcciones), "m" (contorno), "m" (W), "m" (H)
        : "memory"

    );

    return nPuntos;
}


int imageprocess::contornoAPoligono(Point contorno[5000], int nPuntos, Point poligono[5000], float distancia)
{
    int nP = 0;

    // %rsi = contorno; %eax = nPuntos; %rdi = poligono; %edx = nP

    asm volatile(
        "mov $0, %%edx;" //nP = 0
        "flds %4;" //distancia en %st(0) para implementación con FPU
        "movss %4, %%xmm0;" //distancia en %xmm0 para implementación con SSE
        "call contornoAPoligonoRecursivo;"
        "fstp %%st(0);"
        "jmp finContornoAPoligono;"

    "contornoAPoligonoRecursivo:"
        // IMPLEMENTACIÓN DEL PROCEDIMIENTO contornoAPoligonoRecursivo

        "ret;"

        "finContornoAPoligono:"

        : "=d" (nP)
        : "S" (contorno), "a" (nPuntos), "D" (poligono), "m" (distancia)
        : "%xmm0", "memory"
    );


    return nP;

}







