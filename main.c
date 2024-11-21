#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <stdio.h>
#include <stdlib.h>
#include "TQueue.h"

#define PORT "COM3"

HANDLE hSerial;
char* buffer = NULL;
char* command = NULL;

struct TQueue buffer_queue;

int UART_Init(HANDLE hSerial)
{
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    if (!GetCommState(hSerial, &dcbSerialParams))
    {
        printf("Unable to get terminal attributes\n");
        return 1;
    }

    dcbSerialParams.BaudRate = CBR_57600;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams))
    {
        printf("Unable to set terminal attributes\n");
        return 1;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts))
    {
        printf("Unable to set timeouts\n");
        return 1;
    }

    return 0;
}

BOOL WINAPI signal_exit_handler(DWORD dwCtrlType)
{
    printf("Caught signal %d\n", dwCtrlType);
    if (hSerial != INVALID_HANDLE_VALUE)
    {
        printf("Detected opened port, closing now...");
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        printf("CLOSED\n");
    }
    if (buffer != NULL)
    {
        printf("Detected allocated memory, releasing now...");
        free(buffer);
        buffer = NULL;
        printf("RELEASED\n");
    }
    if (command != NULL)
    {
        printf("Detected allocated memory, releasing now...");
        free(command);
        command = NULL;
        printf("RELEASED\n");
    }
    queue_destroy(&buffer_queue);
    printf("Sanity checked\n");
    printf("Exiting...\n");
    ExitProcess(dwCtrlType);
    return TRUE;
}

char str_num_checker(char* num)
{
    unsigned int digits = 0;
    unsigned char changed = 0;
    while (num[digits] != '\0')
    {
        digits++;
    }
    if (digits == 0)
    {
        return 1;
    }
    for (unsigned int i = 0; i < digits; i++)
    {
        if (!isdigit(num[i]))
        {
            return 1;
        }
    }
    for (unsigned int i = 0; i < digits; i++)
    {
        if (num[0] == '0')
        {
            changed = 1;
            for (unsigned int j = 1; j <= digits - i; j++)
            {
                num[j - 1] = num[j];
            }
        }
        else
        {
            break;
        }
    }
    if (changed)
    {
        return -1;
    }
    return 0;
}

unsigned int count_digits(const unsigned int num)
{
    if (num / 10 == 0)
    {
        return 1;
    }
    return 1 + count_digits(num / 10);
}

void SetVolume(float volume)
{
    HRESULT hr;
    CoInitialize(NULL);
    IMMDeviceEnumerator* pEnumerator = NULL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr))
    {
        printf("Unable to create device enumerator\n");
        return;
    }

    IMMDevice* pDevice = NULL;
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr))
    {
        printf("Unable to get default audio endpoint\n");
        pEnumerator->Release();
        return;
    }

    IAudioEndpointVolume* pVolume = NULL;
    hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&pVolume);
    if (FAILED(hr))
    {
        printf("Unable to activate audio endpoint volume\n");
        pDevice->Release();
        pEnumerator->Release();
        return;
    }

    hr = pVolume->SetMasterVolumeLevelScalar(volume, NULL);
    if (FAILED(hr))
    {
        printf("Unable to set master volume level\n");
    }

    pVolume->Release();
    pDevice->Release();
    pEnumerator->Release();
    CoUninitialize();
}

int main()
{
    SetConsoleCtrlHandler(signal_exit_handler, TRUE);

    hSerial = CreateFile(PORT, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hSerial == INVALID_HANDLE_VALUE)
    {
        printf("Unable to open port, is HW connected? Check it, and try again.\n");
        return 1;
    }
    printf("Port open successfully\n");
    if (UART_Init(hSerial) != 0)
    {
        printf("Unable to initialize UART\n");
        CloseHandle(hSerial);
        return 1;
    }

    // Handshake
    const char welcome = 'w';
    Sleep(2000);
    printf("Sending welcome byte now...");
    DWORD bytes_written;
    WriteFile(hSerial, &welcome, 1, &bytes_written, NULL);

    char rec_byte = '0';
    do
    {
        DWORD bytes_read;
        ReadFile(hSerial, &rec_byte, 1, &bytes_read, NULL);
    }
    while (rec_byte != welcome);
    printf("Connection established, welcome byte OK\n");

    queue_init(&buffer_queue);

    unsigned int full_num_count = 0;
    unsigned int last_display_value = 0;
    char coldstart = 1;

    // Main loop
    while (1)
    {
        DWORD bytes_available;
        COMSTAT status;
        ClearCommError(hSerial, NULL, &status);
        bytes_available = status.cbInQue;

        if (bytes_available > 0)
        {
            buffer = (char*)malloc(bytes_available * sizeof(char));
            if (buffer == NULL)
            {
                printf("Memory allocation failed\n");
                break;
            }

            Sleep(5);

            DWORD num_bytes;
            ReadFile(hSerial, buffer, bytes_available, &num_bytes, NULL);
            printf("Allocated %d bytes, Read %d bytes\n", bytes_available, num_bytes);
            printf("Read: ");
            for (DWORD i = 0; i < num_bytes; i++)
            {
                printf("%c", buffer[i]);
                queue_push(&buffer_queue, buffer[i]);
                if (buffer[i] == '\n')
                    full_num_count++;
            }
            free(buffer);
            buffer = NULL;
            printf("\n");
        }

        if (full_num_count != 0)
        {
            char* new_val = malloc(5 * sizeof(char));
            for (int i = 0; i < 5; ++i)
            {
                new_val[i] = '\n';
            }

            unsigned char iter = 0;
            char number_complete = 0;

            do
            {
                queue_front(&buffer_queue, &new_val[iter]);
                queue_pop(&buffer_queue);
                if (new_val[iter] == '\n')
                {
                    number_complete = 1;
                    full_num_count--;
                }
                iter++;
                if (iter == 6)
                {
                    printf("Number runaway\n");
                    while (queue_front(&buffer_queue, &new_val[iter]) == '\n')
                    {
                        queue_pop(&buffer_queue);
                    }
                    free(new_val);
                    new_val = NULL;
                    continue;
                }
            }
            while (!number_complete);

            int pos = 0;
            char is_buf_corrupted = 0;
            while (new_val[pos] != '\n')
            {
                pos++;
                if (pos == 5)
                {
                    printf("Error while searching number's end byte, skipping iteration\n");
                    is_buf_corrupted = 1;
                    break;
                }
            }

            if (!is_buf_corrupted)
            {
                char buf_b_p[pos + 1];

                for (int i = 0; i < pos; i++)
                {
                    buf_b_p[i] = new_val[i];
                }

                buf_b_p[pos] = '\0';

                char is_num_corrupted = str_num_checker(buf_b_p);
                if (is_num_corrupted == -1)
                {
                    printf("Pos of end byte changed, recalculating\n");
                    pos = 0;
                    while (buf_b_p[pos] != '\0')
                    {
                        pos++;
                        if (pos == bytes_available)
                        {
                            printf("Error while searching number's end byte, skipping iteration\n");
                            is_buf_corrupted = 1;
                            break;
                        }
                    }
                    is_num_corrupted = 0;
                    if (pos == 0)
                    {
                        is_num_corrupted = 1;
                    }
                }

                if (!is_num_corrupted)
                {
                    unsigned int adc_val = (int)strtol(buf_b_p, NULL, 10);

                    unsigned int volume = (100 * adc_val) / 1024;

                    if (volume > 100)
                    {
                        volume = 100;
                    }

                    printf("Num OK\n");

                    if (volume != last_display_value || coldstart)
                    {
                        char bytes_to_send[count_digits(volume) + 2];
                        snprintf(bytes_to_send, count_digits(volume) + 2, "%d\n", volume);
                        DWORD bytes_written;
                        WriteFile(hSerial, bytes_to_send, count_digits(volume) + 1, &bytes_written, NULL);
                        last_display_value = volume;
                        coldstart = 0;
                    }

                    SetVolume(volume / 100.0f);
                }
                else
                {
                    printf("Number corrupted, skipping\n");
                }

            }
            else
            {
                printf("Buffer corrupted, skipping\n");
            }
            free(new_val);
            new_val = NULL;
            printf("\n");
            printf("-----------------------------------------");
            printf("-----------------------------------------\n");
            printf("-----------------------------------------");
            printf("-----------------------------------------\n");
            printf("\n");
        }
    }

    signal_exit_handler(0);
    return 0;
}