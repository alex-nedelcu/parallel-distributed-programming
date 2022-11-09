#include <memory>
#include <thread>
#include <algorithm>
#include <vector>
#include "io_operations.cpp"

using std::mutex;
using std::thread;
using std::min;
using std::unique_ptr;
using std::vector;
using std::endl;

// ids
int bill_id_base;
int sold_item_id_base;

// synchronization
std::unique_ptr<std::mutex[]> products_mutexes;
mutex bills_mutex;
mutex sold_item_id_mutex;
mutex global_mutex;

// store input data
unordered_map<int, Product> _products;
unordered_map<int, Product> _products_initial_snapshot;
unordered_map<int, vector<pair<int, int>>> _sale_operations;

// store output data - used for consistency check
int money;
unordered_map<int, Bill> _bills; // generated per sale operation
list<SoldItem> _sold_items; // registers step by step the products that are being sold

/**
 * Worker function for threads that execute sale operations.
 * @param product_quantity_pairs vector of <product_id, quantity> pairs representing the composing parts
 * of the sale operation performed by current thread
 * e.g. [<0, 4>, <3, 25>] - 4 pieces of product with id 0, 25 pieces of product with id 3
 */
void perform_sale_operation(const vector<pair<int, int>>& product_quantity_pairs) {
    Bill bill; // bill corresponding to the currently performed sale operation

    bills_mutex.lock();
    int bill_id = bill_id_base++;
    bills_mutex.unlock();

    bill.id = bill_id;
    bill.total_price = 0;

    bills_mutex.lock();
    _bills[bill_id] = bill;
    bills_mutex.unlock();


    // loop through the components of the current sale operation
    for (auto const& pair: product_quantity_pairs) {
        auto product_id = pair.first;
        auto requested_quantity = pair.second;

        products_mutexes[product_id].lock();

        auto product = _products[product_id];
        requested_quantity = min(requested_quantity, product.quantity);

        SoldItem sold_item;

        sold_item_id_mutex.lock();
        sold_item.id = sold_item_id_base++;
        sold_item_id_mutex.unlock();

        sold_item.bill_id = bill.id;
        sold_item.product_id = product_id;
        sold_item.quantity = requested_quantity;

        global_mutex.lock(); // the 3 instructions below are critical and should not be interrupted by consistency check job

        money += sold_item.quantity * product.unit_price;
        _bills[bill_id].total_price += sold_item.quantity * product.unit_price;
        _sold_items.push_back(sold_item);
        _products[product_id].quantity -= requested_quantity;

        global_mutex.unlock();

        products_mutexes[product_id].unlock();
    }
}

void perform_consistency_check() {
    global_mutex.lock();

    unordered_map<int, int> sold_quantity_per_product; // <product_id, sold_quantity>
    auto expected_money = 0;

    // calculate sold quantity for each product based on created sold items
    for (auto const& sold_item: _sold_items) {
        auto product = _products[sold_item.product_id];

        if (sold_quantity_per_product.find(product.id) != sold_quantity_per_product.end()) {
            sold_quantity_per_product[product.id] += sold_item.quantity;
        } else {
            sold_quantity_per_product[product.id] = sold_item.quantity;
        }

        expected_money += product.unit_price * sold_item.quantity;
    }

    bool is_money_consistent = expected_money == money;
    bool are_quantities_consistent = true;

    for (auto const& pair: _products_initial_snapshot) {
        auto product = pair.second;
        auto initial_quantity = product.quantity;
        auto sold_quantity = sold_quantity_per_product[product.id];

        if (initial_quantity - sold_quantity != _products[product.id].quantity) {
            are_quantities_consistent = false;
            break;
        }
    }

    if (is_money_consistent && are_quantities_consistent) {
        cout << "SUCCESS: CONSISTENCY CHECK PASSED" << "\n";
    } else {
        cout << "ERROR: CONSISTENCY CHECK FAILED" << "\n";
        cout << "-----------------" << endl;
        cout << "Expected money: " << expected_money << endl;
        cout << "Actual money: " << money << endl;
        cout << "Quantities are consistent: " << are_quantities_consistent << endl;
        cout << "----------------------------" << endl;
    }

    global_mutex.unlock();
}

void initialize_global_variables() {
    money = 0;
    bill_id_base = 0;
    sold_item_id_base = 0;
};

void initialize_products_mutexes(unsigned long size) {
    products_mutexes = std::make_unique<std::mutex[]>(size);
}

int main() {
    _products = read_products_from_file("/Users/alexnedelcu/Desktop/Uni/PPD/lab1/products.txt");
    _products_initial_snapshot = _products;
    _sale_operations = read_sale_operations_from_file("/Users/alexnedelcu/Desktop/Uni/PPD/lab1/sale_operation_components.txt");

    initialize_global_variables();
    initialize_products_mutexes(_products.size());

    thread sale_operation_threads[_sale_operations.size()];
    auto sale_operation_thread_counter = 0;

    thread consistency_check_threads[_sale_operations.size()];
    auto consistency_check_thread_counter = 0;

    for (auto const& pair: _sale_operations) {
        auto product_quantity_pairs = pair.second;

        sale_operation_threads[sale_operation_thread_counter++] = thread(perform_sale_operation, product_quantity_pairs);

        if (sale_operation_thread_counter % 3 == 0) {
            consistency_check_threads[consistency_check_thread_counter++] = thread(perform_consistency_check);
        }
    }

    for (int i = 0; i < sale_operation_thread_counter; i++) {
        sale_operation_threads[i].join();
    }

    for (int i = 0; i < consistency_check_thread_counter; i++) {
        consistency_check_threads[i].join();
    }

    return 0;
}
