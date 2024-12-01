#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "TQueue.h"

#define PORT "/dev/ttyACM0"

// Global variables
int port = -1; // Port file descriptor
char* buffer = nullptr; // Buffer for reading data
char* command = nullptr; // Command string

struct TQueue buffer_queue; // Queue for buffering data

// Function to initialize UART communication
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

    // Set timeout
    Serial.c_cc[VMIN] = 0;
    Serial.c_cc[VTIME] = 5;

    tcflush(port, TCIOFLUSH); // Flush input and output registers

    if (tcsetattr(port, TCSANOW, &Serial))
    {
        printf("Unable to set terminal attributes\n");
        return 1;
    }
    return 0;
}

// Signal handler for cleanup on exit
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

// Function to check if a string is a valid number
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
        if (num[0] == '0' && digits > 1)
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

// Function to count the number of digits in a number
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
    // Set up signal handlers for cleanup on exit
    signal(SIGTERM, signal_exit_handler);
    signal(SIGINT, signal_exit_handler);
    signal(SIGQUIT, signal_exit_handler);
    signal(SIGHUP, signal_exit_handler);
    signal(SIGSEGV, signal_exit_handler);
    signal(SIGFPE, signal_exit_handler);
    signal(SIGKILL, signal_exit_handler);
    signal(SIGPIPE, signal_exit_handler);

    int running = 1;

    // Open the port
    port = open(PORT, O_RDWR | O_NOCTTY);
    if (port < 0)
    {
        printf("Unable to open port, is HW connected? Check it, and try again.\n");
        return 1;
    }
    printf("Port open successfully\n");

    // Initialize UART
    if (UART_Init(port) != 0)
    {
        printf("Unable to initialize UART\n");
        close(port);
        return 1;
    }

    // Handshake with the device
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

    // Initialize the buffer queue
    queue_init(&buffer_queue);

    unsigned int full_num_count = 0;

    // Main loop
    while (running)
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

        // Handle complete numbers in the buffer
        while (full_num_count != 0)
        {
            char* new_val = (char*)malloc(5 * sizeof(char));
            if (new_val == nullptr)
            {
                printf("Memory allocation failed\n");
                break;
            }

            unsigned char iter = 0;
            char number_complete = 0;

            // Read characters from the queue until a complete number is found
            while (!number_complete)
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
                    break;
                }
            }

            if (number_complete)
            {
                // Process the complete number
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
                            command = (char*)malloc(21 * sizeof(char)); // 21
                            if (command == nullptr)
                            {
                                printf("Memory allocation for command failed\n");
                                signal_exit_handler(-1);
                            }
                            size = 21;
                            break;
                        case 2:
                            command = (char*)malloc(22 * sizeof(char)); // 22
                            if (command == nullptr)
                            {
                                printf("Memory allocation for command failed\n");
                                signal_exit_handler(-1);
                            }
                            size = 22;
                            break;
                        case 3:
                            command = (char*)malloc(23 * sizeof(char)); // 23
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
                printf("---------------------------------------\n");
                printf("-----------------------------------------");
                printf("---------------------------------------\n");
                printf("\n");
            }
        }
    }

    signal_exit_handler(0); // Cleanup before exiting
    return 0;
}