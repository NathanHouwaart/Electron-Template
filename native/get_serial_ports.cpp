#include <windows.h>
#include <string>
#include <iostream>


int main(){
    char lpTargetPath[5000]; // buffer to store the path of the COMPORTS
         for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
         {
             std::string str = "COM" + std::to_string(i); // converting to COM0, COM1, COM2
             DWORD test = QueryDosDeviceA(str.c_str(), lpTargetPath, 5000);

            // std::cout << "COM Port: " << str << " - ";

             // Test the return value and error if any
             if (test != 0) //QueryDosDevice returns zero if it didn't find an object
             {
                 std::cout << "Found: " << str << std::endl;
             }
    
             if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
             {
                 std::cout << "Buffer insufficient for: " << str << std::endl;
             }
         }

}
  