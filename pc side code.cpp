#include <iostream>
#include <string>
#include <serial/serial.h>
#include <chrono>

using namespace std;

// Function to calculate CRC checksum
uint16_t calculate_crc(const string &data)
{
    const uint16_t polynomial = 0x8005;
    uint16_t crc = 0xFFFF; // Initial CRC value

    for (char c : data)
    {
        crc ^= (static_cast<uint16_t>(c) << 8);

        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
            {
                crc = (crc << 1) ^ polynomial;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

int main()
{

    serial::Serial ser("COM5", 2400, serial::Timeout::simpleTimeout(1000));

    if (!ser.isOpen())
    {
        cerr << "Error opening serial port." << endl;
        return 1;
    }

    // Text to be transmitted
    string data_to_send =
        "Finance Minister Arun Jaitley Tuesday hit out at former RBI governor Raghuram Rajan for predicting "
        "that the next banking crisis would be triggered by MSME lending, saying postmortem is easier than "
        "taking action when it was required. Rajan, who had as the chief economist at IMF warned of impending "
        "financial crisis of 2008, in a note to a parliamentary committee warned against ambitious credit targets "
        "and loan waivers, saying that they could be the sources of next banking crisis. Government should focus "
        "on sources of the next crisis, not just the last one. In particular, government should refrain from "
        "setting ambitious credit targets or waiving loans. Credit targets are sometimes achieved by abandoning "
        "appropriate due diligence, creating the environment for future NPAs,\" Rajan said in the note.\" Both "
        "MUDRA loans as well as the Kisan Credit Card, while popular, have to be examined more closely for potential "
        "credit risk. Rajan, who was RBI governor for three years till September 2016, is currently.";

    // Chunk size for data transmission
    const size_t chunk_size = 32;

    // Calculate CRC checksum for the entire data
    uint16_t crc_full = calculate_crc(data_to_send);

    // Timer and counter for real-time speed calculation
    auto start_time = chrono::high_resolution_clock::now();
    size_t total_bits_sent = 0;

    // Split data into chunks and send with CRC
    for (size_t i = 0; i < data_to_send.length(); i += chunk_size)
    {
        string chunk = data_to_send.substr(i, chunk_size);

        // Calculate CRC for the chunk
        uint16_t crc_chunk = calculate_crc(chunk);

        // Add CRC to the data
        string data_with_crc = chunk + "," + to_string(crc_chunk) + "\n";

        // Send data to MCU
        ser.write(data_with_crc);

        // Update total bits sent
        total_bits_sent += data_with_crc.size() * 8;

        // Calculate elapsed time
        auto end_time = chrono::high_resolution_clock::now();
        auto elapsed_time = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count() / 1000.0;

        // Calculate and print real-time speed
        double speed = total_bits_sent / elapsed_time;
        cout << "Real-time Speed: " << speed << " bits/second" << endl;

        // Wait for a short time to simulate real-time transmission
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    // Signal end of transmission
    ser.write("END\n");

    // Receive and print data sent back by MCU
    while (ser.available())
    {
        cout << ser.read(ser.available());
    }

    // Close the serial port
    ser.close();

    return 0;
}
