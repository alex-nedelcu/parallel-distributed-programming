#include <iostream>
#include <thread>
#include <vector>

using std::mutex;
using std::condition_variable;
using std::unique_lock;
using std::lock_guard;
using std::thread;
using std::vector;
using std::endl;
using std::cin;
using std::cout;

// synchronization
mutex _mutex;
condition_variable new_partial_product_generated;

// input data
vector<int> first_vector{6, 1, -3};
vector<int> second_vector{-1, 6, -5};

// output data
int final_scalar_product = 0;
vector<int> partial_products;
bool generated = false;
bool finished = false;
int partial_products_index = 0;


void produce_partial_product() {
    for (int index = 0; index < first_vector.size(); index++) {
        _mutex.lock();

        // produces new data
        partial_products.push_back(first_vector.at(index) * second_vector.at(index));
        generated = true;

        _mutex.unlock();

        // notifies consumer
        new_partial_product_generated.notify_one();
    }

    finished = true;
}

void add_partial_product_to_final_result() {
    while (!finished) {
        unique_lock<mutex> lock(_mutex);

        // waits for condition variable to change
        while (!generated) {
            new_partial_product_generated.wait(lock);
        }

        // consumes data
        while (partial_products_index < partial_products.size()) {
            final_scalar_product += partial_products.at(partial_products_index);
            partial_products_index++;
        }

        generated = false;

        lock.unlock();
    }
}

int main() {
    thread partial_product_consumer(add_partial_product_to_final_result);
    thread partial_product_producer(produce_partial_product);

    partial_product_consumer.join();
    partial_product_producer.join();

    cout << "[A] Result is " << final_scalar_product << endl;

    return 0;
}
