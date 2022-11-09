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
int shared_partial_product;
bool generated = false;
bool finished = false;


void produce_partial_product() {
    for (int index = 0; index < first_vector.size(); index++) {
        unique_lock<mutex> lock(_mutex);

        // waits until 'generated' is false
        new_partial_product_generated.wait(lock, []() {return !generated;});

        // produces new data
        shared_partial_product = first_vector.at(index) * second_vector.at(index);
        generated = true;

        lock.unlock();

        // notifies consumer
        new_partial_product_generated.notify_one();
    }

    finished = true;
}

void add_partial_product_to_final_result() {
    while (!finished) {
        unique_lock<mutex> lock(_mutex);

        // waits until 'generated' becomes true
        new_partial_product_generated.wait(lock, []() {return generated;});

        // consumes data
        if (generated) {
            final_scalar_product += shared_partial_product;
            generated = false;
        }

        lock.unlock();

        // notify the producer to produce next partial product
        new_partial_product_generated.notify_one();
    }
}

int main() {
    thread partial_product_consumer(add_partial_product_to_final_result);
    thread partial_product_producer(produce_partial_product);

    partial_product_consumer.join();
    partial_product_producer.join();

    cout << "[B] Result is " << final_scalar_product << endl;

    return 0;
}
