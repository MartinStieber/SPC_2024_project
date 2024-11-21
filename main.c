#include <windows.h>
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
                    unsigned int size = 0;
                    switch (count_digits(volume))
                    {
                    case 1:
                        command = (char*)malloc(21 * sizeof(char));
                        if (command == NULL)
                        {
                            printf("Memory allocation for command failed\n");
                            signal_exit_handler(-1);
                        }
                        size = 21;
                        break;
                    case 2:
                        command = (char*)malloc(22 * sizeof(char));
                        if (command == NULL)
                        {
                            printf("Memory allocation for command failed\n");
                            signal_exit_handler(-1);
                        }
                        size = 22;
                        break;
                    case 3:
                        command = (char*)malloc(23 * sizeof(char));
                        if (command == NULL)
                        {
                            printf("Memory allocation for command failed\n");
                            signal_exit_handler(-1);
                        }
                        size = 23;
                        break;
                    default:
                        printf("Invalid command length: %d\n", pos);#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "TQueue.h"

#define PORT "/dev/ttyACM0"

int port = -1; // Globální proměnná pro port
char* buffer = nullptr; // Globální ukazatel pro buffer
char* command = nullptr; // Globální ukazatel pro string příkazu

struct TQueue buffer_queue;

int UART_Init(const int port)
{
    struct termios Serial;
    if (tcgetattr(port, &Serial))
    {
        printf("Unable to get terminal attributes\n");
        return 1;
    }
    cfsetispeed(&Serial, B57600);
    cfsetospeed(&Serial, B57600);

    Serial.c_cflag &= ~CSIZE;
    Serial.c_cflag |= CS8;
    Serial.c_cflag &= ~(PARENB | CSTOPB);

    Serial.c_iflag &= ~(IXON | IXOFF | IXANY);
    Serial.c_iflag &= ~(INLCR | ICRNL);

    Serial.c_oflag &= ~OPOST;

    Serial.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Nastavení timeoutu
    Serial.c_cc[VMIN] = 0;
    Serial.c_cc[VTIME] = 5;

    tcflush(port, TCIOFLUSH); //Flush input and output register

    if (tcsetattr(port, TCSANOW, &Serial))
    {
        printf("Unable to set terminal attributes\n");
        return 1;
    }
    return 0;
}

void signal_exit_handler(const int signum)
{
    printf("Caught signal %d\n", signum);
    if (port >= 0)
    {
        printf("Detected opened port, closing now...");
        close(port);
        port = -1;
        printf("CLOSED\n");
    }
    if (buffer != nullptr)
    {
        printf("Detected allocated memory, releasing now...");
        free(buffer);
        buffer = nullptr;
        printf("RELEASED\n");
    }
    if (command != nullptr)
    {
        printf("Detected allocated memory, releasing now...");
        free(command);
        command = nullptr;
        printf("RELEASED\n");
    }
    queue_destroy(&buffer_queue);
    printf("Sanity checked\n");
    printf("Exiting...\n");
    exit(signum);
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

int main()
{
    // Nastavení handlerů signálů
    signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGQUIT, signal_exit_handler);
    signal(SIGHUP, signal_exit_handler);
    signal(SIGSEGV, signal_exit_handler);
    signal(SIGFPE, signal_exit_handler);
    signal(SIGKILL, signal_exit_handler);
    signal(SIGPIPE, signal_exit_handler);

    int running = 1;

    port = open(PORT, O_RDWR | O_NOCTTY);
    if (port < 0)
    {
        printf("Unable to open port, is HW connected? Check it, and try again.\n");
        return 1;
    }
    printf("Port open successfully\n");
    if (UART_Init(port) != 0)
    {
        printf("Unable to initialize UART\n");
        close(port);
        return 1;
    }

    //Handshake
    constexpr char welcome = 'w';
    sleep(2);
    printf("Sending welcome byte now...");
    write(port, &welcome, 1);

    char rec_byte = '0';
    do
    {
        int bytes_available = 0;
        ioctl(port, FIONREAD, &bytes_available);
        if (bytes_available > 0)
        {
            read(port, &rec_byte, 1);
        }
    }
    while (rec_byte != welcome);
    printf("Connection established, welcome byte OK\n");

    struct TQueue buffer_queue;
    queue_init(&buffer_queue);

    unsigned int full_num_count = 0;

    unsigned int last_display_value = 0;

    char coldstart = 1;

    //Main loop
    do
    {
        int bytes_available = 0;
        if (ioctl(port, FIONREAD, &bytes_available) == -1)
        {
            printf("Error while checking bytes available\n");
            break;
        }

        if (bytes_available > 0)
        {
            buffer = (char*)malloc(bytes_available * sizeof(char));
            if (buffer == nullptr)
            {
                printf("Memory allocation failed\n");
                break;
            }

            usleep(5000);

            const int num_bytes = (int)read(port, buffer, bytes_available);
            if (num_bytes < 0)
            {
                printf("Error while reading bytes\n");
                break;
            }
            printf("Allocated %d bytes, Read %d bytes\n", bytes_available, num_bytes);
            printf("Read: ");
            for (int i = 0; i < num_bytes; i++)
            {
                printf("%c", buffer[i]);
                queue_push(&buffer_queue, buffer[i]);
                if (buffer[i] == '\n')
                    full_num_count++;
            }
            free(buffer);
            buffer = nullptr;
            printf("\n");
        }

    NUM_RUNAW_HANDL:

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
                    new_val = nullptr;
                    goto NUM_RUNAW_HANDL;
                }
            }
            while (!number_complete);


            //Prikaz
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
                    unsigned int adc_val = (int)strtol(buf_b_p, nullptr, 10);

                    unsigned int volume = (100 * adc_val) / 1024;

                    if (volume > 100)
                    {
                        volume = 100;
                    }


                    printf("Num OK\n");
                    unsigned int size = 0;
                    switch (count_digits(volume))
                    {
                    case 1:
                        command = (char*)malloc(21 * sizeof(char)); //21
                        if (command == nullptr)
                        {
                            printf("Memory allocation for command failed\n");
                            signal_exit_handler(-1);
                        }
                        size = 21;
                        break;
                    case 2:
                        command = (char*)malloc(22 * sizeof(char)); //22
                        if (command == nullptr)
                        {
                            printf("Memory allocation for command failed\n");
                            signal_exit_handler(-1);
                        }
                        size = 22;
                        break;
                    case 3:
                        command = (char*)malloc(23 * sizeof(char)); //23
                        if (command == nullptr)
                        {
                            printf("Memory allocation for command failed\n");
                            signal_exit_handler(-1);
                        }
                        size = 23;
                        break;
                    default:
                        printf("Invalid command length: %d\n", pos);
                        signal_exit_handler(-1);
                    }

                    if (volume != last_display_value || coldstart)
                    {
                        char bytes_to_send[count_digits(volume) + 2];
                        snprintf(bytes_to_send, count_digits(volume) + 2, "%d\n", volume);
                        write(port, bytes_to_send, count_digits(volume) + 1);
                        last_display_value = volume;
                        coldstart = 0;
                    }

                    snprintf(command, size, "amixer set Master %d%%", volume);
                    printf("\n");
                    const int result = system(command);
                    if (result != 0)
                    {
                        printf("Error while executing command\n");
                    }

                    free(command);
                    command = nullptr;
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
            new_val = nullptr;
            printf("\n");
            printf("-----------------------------------------");
            printf("-----------------------------------------\n");
            printf("-----------------------------------------");
            printf("-----------------------------------------\n");
            printf("\n");
        }
    }
    while (running);

    signal_exit_handler(0); // Uvolnění prostředků před ukončením
    return 0;
}

                        signal_exit_handler(-1);
                    }

                    if (volume != last_display_value || coldstart)
                    {
                        char bytes_to_send[count_digits(volume) + 2];
                        snprintf(bytes_to_send, count_digits(volume) + 2, "%d\n", volume);
                        DWORD bytes_written;
                        WriteFile(hSerial, bytes_to_send, count_digits(volume) + 1, &bytes_written, NULL);
                        last_display_value = volume;
                        coldstart = 0;
                    }

                    snprintf(command, size, "amixer set Master %d%%", volume);
                    printf("\n");
                    const int result = system(command);
                    if (result != 0)
                    {
                        printf("Error while executing command\n");
                    }

                    free(command);
                    command = NULL;
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