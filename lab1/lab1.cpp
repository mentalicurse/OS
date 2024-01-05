#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

mutex mtx;            //объявляем мьютекс
condition_variable cdv; // объявляем условную переменную

int flag = 0;            //объявляем флаг события

void Provider() {    //функция потока-поставщика
    while (1) {    //выполняем бесконечный цикл
        this_thread::sleep_for(chrono::milliseconds(1000)); //поток засыпает на 1 секунду
        unique_lock<mutex> ul(mtx);            //захватываем мьютекс
        if (flag == 1) {                        //если флаг события установлен
            cout << "Поставщик: событие не обработанно" << endl; // выводим сообщение об этом
            ul.unlock();        //освобождаем мьютекс
            continue;            //переходим к следующей итерации цикла
        }
        flag = 1;        //устанавливаем флаг события
        cout << "Поставщик: сообщение отправлено" << endl;    //выводим сообщение об отправке
        cdv.notify_one();    //сообщаем потребителю о событии
        ul.unlock();        //освобождаем мьютекс
    }
}

void Consumer() {                //функция потока-потребителя
    while (1) {                //выполняем бесконечный цикл
        unique_lock<mutex> ul(mtx);    //захватываем мьютекс
        while (flag == 0) {            //пока флаг события не установлен
            cdv.wait(ul);            //ждем сигнала от потока-поставщика
        }
        flag = 0;                    //сбрасываем флаг события
        cout << "Получатель: сообщение получено" << endl;    //выводим сообщение о получении
        ul.unlock();                //особождаем мьютекс
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
