#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

mutex mtx;            //объявляем мьютекс
condition_variable cdv; // объявляем условную переменную

int flag = 0;            //объявляем флаг события

void Provider() {    //функция потока-поставщика
    while (1) {    
        this_thread::sleep_for(chrono::milliseconds(1000)); 
        unique_lock<mutex> ul(mtx);            
        if (flag == 1) {                        
            cout << "Поставщик: событие не обработанно" << endl; 
            ul.unlock();        
            continue;            
        }
        flag = 1;        
        cout << "Поставщик: сообщение отправлено" << endl;    
        cdv.notify_one();    
        ul.unlock();        
    }
}

void Consumer() {                //функция потока-потребителя
    while (1) {                
        unique_lock<mutex> ul(mtx);    
        while (flag == 0) {            
            cdv.wait(ul);            
        }
        flag = 0;                    
        cout << "Получатель: сообщение получено" << endl;    
        ul.unlock();                
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    thread prod_th(Provider); // Создаем поток-поставщик
    thread cons_th(Consumer); // Создаем поток-потребитель

    //ждем завершения обоих потоков
    prod_th.join();
    cons_th.join(); 

    return 0;    //возвращаем 0
}
