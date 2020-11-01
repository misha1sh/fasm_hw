; Шестаков Михаил Сергеевич
; Группа БПИ196

format PE console
entry start

include 'win32a.inc'

section '.data' data readable writable
        strAskString            db   'Input string: ', 0
        
        ; маска для разворота 16 байт в обратную сторону
        shuffleOrder            db   15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
        
        strFormatInputStr       db   'Initial  string: "%s"', 10, 0
        strFormatResultStr      db   'Reversed string: "%s"', 10, 0
        strFormatInt db '%d', 10, 0
        strPointer              dd   0

        
        
section '.code' code readable executable
    start:
    
        cinvoke printf, strAskString ; просим пользователя ввести строку
        
        ccall ReadString             ; читаем строку
        mov [strPointer], eax

        cinvoke printf, strFormatInputStr, [strPointer] ; выводим исходную строку
        

        ccall ReverseString, strPointer ; переворачиваем строку на месте
        
        cinvoke printf, strFormatResultStr, [strPointer] ; выводим результат (перевёрнутая строка)
        
        cinvoke getch
        invoke ExitProcess, 0
    
    
    
    ; char* ReadString(); -- читает строку (до перевода строки) с использованием WinApi
    proc ReadString c uses edi
        locals
            s                     dd  0
            s_size                dd  0       ; изначально строка пустая
            handle                dd  0
        endl
        
        cinvoke __iob_func ; полуаем FILE* stdin
        mov [handle], eax

        
        cinvoke calloc, [s_size]  ; выделяем память под строку
        mov [s], eax
        
        mov ecx, -1
    .readLoop:      
        mov eax, [s_size] ; считываем размер
        add eax, 1024 ; увличиваем буффер на 1024 символа
        mov [s_size], eax ; сохраняем новый размер

        
        cinvoke realloc, [s], eax ; увлечиваем строку
        mov [s], eax         ; сохраняем указатель на начало строки

    
        
        mov ecx, [s_size] 
        lea edi, [eax + ecx - 1024] ; edi -- указатель на символ, начиная с котрого мы считываем новую часть строки

        cinvoke fgets, edi, 1025, [handle] ; указываем 1025, чтобы fgets прочитал 1024 символа
        cmp eax, 0 ; проверяем на налчие ошибок
        je .endRead ; если ничего не удалось прочитать, то выходим

        mov ecx, 1024
        mov al, 10 ; символ перевода строки
        ; edi всё ещё указывает на символ, начиная с которого мы читали
        repnz SCASB
        ; если zf = 1, значит мы встретили перевод строки, edi указывает на символ после него, тогда выходим
    LOOPNE .readLoop ; zf = 0

    
    .endLoop: ; edi указывает на \n, s указывает на символ после последнего
        dec edi ; возвращаем edi на символ перевода строки
        mov ebx, [s]
        
        cmp ebx, edi ; провряем, вдруг пользователь ввёл просто перевод строки (без \r)
        je .endRead
        
        dec edi ; смотрим на предыдущий символ
        cmp byte [edi], 13 ; проверяем, что это \r
        je .endRead ; если да, то обрезаем строку в этом месте
        inc edi ; если нет, то возвращаемся обратно на \n
    
    .endRead: ; edi указывает на символ после последнего
        mov byte [edi], 0 ; устанавливаем нуль-терминатор
        mov eax, [s] ; возвращаем указатель на строку
        ret
    endp
    
    
    
    
    ; void ReverseString(char* str) -- переворачивает строку "на месте"
    proc ReverseString c uses esi edi ebx, \
            stringPointer: DWord
            
        mov esi, [stringPointer]
        mov esi, [esi]   ; помещаем в esi и edi указатель на начало строки
        mov edi, esi
        
        mov ecx, -1             ; мы не знаем сколько символов в строке
        xor al, al              ; al = 0
        cld
        repne SCASB
        sub edi, 2
        neg ecx
        sub ecx, 2
        je .ret ; если строка оказалась пустая, то выходим
        
        ; в итоге в edi лежит указатель на последний символ строки, а в ecx количество символов (без нуль терминатора)
        
        
        
        shr ecx, 5 ; делим на 32, потому что мы будем с использованием SSE3 поворачивать
                   ; по 16 символов за раз в начале и в конце, то есть в сумме за итерацию 32 символа
        
        je .simpleswap ; если ecx равен 0, то переходим к повороту строки без использования SSE
        
        ; загружаем в xmm2 нужный нам порядок замены байт
        mov eax, shuffleOrder
        movups xmm2, [eax]
        
        sub edi, 15 ; сдвигаемся на 15, потому что мы двигаем по 16 за раз
        
        .loop:
            ; загружаем в xmm начало и конец
            movups xmm0, [esi]
            movups xmm1, [edi]
            
            ; поворачиваем начало и конец строки
            ; pshufb --  sse3 инструкция, которая меняет порядок байт в 128й битном
            ; слове src на основе порядка, указанного в dst 
            pshufb xmm0, xmm2
            pshufb xmm1, xmm2
            
            ; вставляем конец в начало, а начало в конец
            movups [esi], xmm1
            movups [edi], xmm0
            
            ; сдвигаем начало и конец
            add esi, 16
            sub edi, 16
        LOOP .loop  
        add edi, 15 ; возвращаем лишний сдвиг
        
        .simpleswap: ; простой разворот строки по байту за раз
            cld  ; на всякий случай очищаем флаг направления
            
            mov ecx, edi ; считаем число операций ecx = (edi - esi + 1) / 2
            inc ecx
            sub ecx, esi
            shr ecx, 1
            
            .loop2:
                mov bl, [edi] ; в bl сохраняем значение из edi
                movsb ; перемещаем символ из esi в edi, после этого esi++, edi++
                sub edi, 2 ; сдвигаем edi в обратную сторону
                mov [esi - 1], bl ; записываем в esi ранее сохранённый символ
                
            LOOP .loop2
        .ret:
            ret
    endp
        
    
;-------------------------------third act - including HeapApi--------------------------
                                                 
section '.idata' import data readable
    library kernel, 'kernel32.dll',\
            msvcrt, 'msvcrt.dll',\
            user32,'USER32.DLL'

include 'api\user32.inc'
include 'api\kernel32.inc'
    import kernel,\
           ExitProcess, 'ExitProcess',\
           HeapCreate,'HeapCreate',\
           ReadConsoleA,'ReadConsoleA',\
           GetStdHandle,'GetStdHandle',\
           HeapAlloc,'HeapAlloc'
  include 'api\kernel32.inc'
    import msvcrt,\
           fgets, 'fgets', \
           malloc, 'malloc', \
           calloc, 'calloc', \
           realloc, 'realloc', \
           strlen, 'strlen', \
           printf, 'printf',\
           scanf, 'scanf',\
           __iob_func, '__iob_func',\
           getch, '_getch'