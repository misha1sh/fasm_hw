; Шестаков Михаил Сергеевич 
; группа БПИ 196
; Вариант 8 ( 28 в списке)
; нужно заменить элементы в массиве меньшие -5, на x - 5, большие 5 на x + 5. элементы между 5 и минус 5 заменить на 0

format PE console
entry start

include 'win32a.inc'

section '.data' data readable writable
        strAskArraySize         db   'array size:', 0
        strAskArrayElement      db   'element %d: ', 0
        strFormatInt            db   '%d', 0
        
        strPrintfIncorrectSize  db   'Size of array should be between 1 and 10000. Got: %d', 10, 0
        
        strPrintfInt            db   '%d, ', 0
        strBracketLeft          db   '[', 0
        strBracketRight         db   ']', 0
        
        strInputArray           db   'Input array: ', 0
        strOutputArray          db   'Output array: ', 0
        strEndOfLine            db   10, 0
        
        in_arr_size             dd   0
        in_arr                  dd   0
        
        out_arr                 dd   0
        
        hHeap                   dd  -1
        

    
section '.code' code readable executable
    start:
	    invoke HeapCreate, 0, 1024, 0
        mov [hHeap], eax
		
        call ReadArray
        
        ccall SolveTask, [in_arr], [in_arr_size]
        mov [out_arr], eax
        
        
        cinvoke printf, strInputArray
        
        push [in_arr_size]
        push [in_arr]
        call PrintArray
        add esp, 8
        
        cinvoke printf, strEndOfLine
        
        cinvoke printf, strOutputArray
        
        push [in_arr_size]
        push [out_arr]
        call PrintArray
        add esp, 8
        
        cinvoke printf, strEndOfLine
        
        
        invoke getch
        invoke ExitProcess, 0
        
    
    ; void ReadArray()
    ReadArray:
        ; read in_arr_size
        invoke printf, strAskArraySize
        invoke scanf, strFormatInt, in_arr_size
        add esp, 12
        
        ; check that in_arr_size > 0
        mov eax, [in_arr_size]
		cmp eax, 10000
		jg incorrectSize
        cmp eax, 0
        jg readElements
	incorrectSize:
    ; in_arr_size <= 0
        invoke printf, strPrintfIncorrectSize, [in_arr_size]
		invoke getch
        invoke ExitProcess, 0       
    readElements:
		imul eax, 4
		
	    invoke HeapAlloc, [hHeap], 0, eax
        mov [in_arr], eax
		
        xor ecx, ecx
        mov ebx, eax
    readElementsLoop:
        cmp ecx, [in_arr_size]
        jge endReading
    
        invoke printf, strAskArrayElement,ecx
        invoke scanf,strFormatInt,ebx
        add esp, 12
        pop ecx
        
        inc ecx
        add ebx, 4
        jmp readElementsLoop
        
    endReading:
        ret
        
        
        
        
        
        
    ; int* SolveTask(int* array, int array_size)
    proc SolveTask c uses esi edi, \
        array: DWord, array_size: DWord 
        locals
            result_array     dd 0
            array_size_bytes dd ?
        endl
        
        mov eax, [array_size]
        imul eax, 4
        mov [array_size_bytes], eax
        

        invoke HeapAlloc, [hHeap], 0, [array_size_bytes]
        mov [result_array], eax
        
        
        mov esi, [array]
        mov edi, [result_array]
        xor ecx, ecx
    solveTaskLoop:
        cmp ecx, [array_size]
        jge endSolveTaskLoop
        
        mov eax, [esi + ecx * 4]
        
        cmp eax, 5
        jg GreatherThanFive
        cmp eax, -5
        jl lowerThanMinusFive
    BetweenMinusFiveAndFive:
        mov Dword[edi + ecx * 4], 0
        jmp continueSolveTaskLoop
    
    GreatherThanFive:
        add eax, 5
        mov Dword[edi + ecx * 4], eax
        jmp continueSolveTaskLoop
    
    lowerThanMinusFive:
        add eax, -5
        mov Dword[edi + ecx * 4], eax
        ; jmp continueSolveTaskLoop
        
    continueSolveTaskLoop:          
        inc ecx
        jmp solveTaskLoop
        
    endSolveTaskLoop:
        
        mov eax, [result_array]
        ret
    endp
    
    
    
    
        
    ; void PrintArray(int* arr, int arr_size) 
    PrintArray:
        
        ; save stack
        push ebp
        mov ebp, esp
        
        invoke printf, strBracketLeft
        add esp, 4
        
        
        ; get parameters
        mov ebx, [ebp+8]   ; arr pointer
        mov eax, [ebp+12]  ; arr size
        
        lea eax, [ebx + 4*eax - 4] ; eax is now points to the end of array - 1
        
    printArrayLoop:
        cmp ebx, eax
        jge printLastElement
        
        push eax
        
        invoke printf, strPrintfInt, [ebx]
        add esp, 8

        pop eax
        add ebx, 4
        jmp printArrayLoop
        
    printLastElement:
        invoke printf, strFormatInt, [ebx]
        add esp, 8
        
    endPrintArray:
        invoke printf, strBracketRight
        add esp, 4
    
        pop ebp
        ret
        
        

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
           HeapAlloc,'HeapAlloc'
  include 'api\kernel32.inc'
    import msvcrt,\
           printf, 'printf',\
           scanf, 'scanf',\
           getch, '_getch'