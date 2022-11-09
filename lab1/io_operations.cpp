#include <unordered_map>
#include <fstream>
#include <iostream>
#include <utility>
#include <list>
#include <vector>
#include "domain.cpp"

using std::unordered_map;
using std::string;
using std::ifstream;
using std::cout;
using std::pair;
using std::list;
using std::make_pair;
using std::vector;

unordered_map<int, Product> read_products_from_file(const string& filename) {
    ifstream file(filename);
    unordered_map<int, Product> products;
    Product product;

    while (file >> product.id >> product.quantity >> product.unit_price) {
        products[product.id] = product;
    }

    return products;
}

unordered_map<int, vector<pair<int, int>>> read_sale_operations_from_file(const string& filename) {
    ifstream file(filename);
    unordered_map<int, vector<pair<int, int>>> sale_operations;
    SaleOperationComponent sale_operation_component;

    while (file >> sale_operation_component.id
                >> sale_operation_component.sale_operation_id
                >> sale_operation_component.product_id
                >> sale_operation_component.quantity
            ) {
        auto sale_operation_id = sale_operation_component.sale_operation_id;
        auto product_quantity_pair = make_pair(sale_operation_component.product_id, sale_operation_component.quantity);

        sale_operations[sale_operation_id].push_back(product_quantity_pair);
    }

    return sale_operations;
}

void print_product(Product product) {
    cout << "Product {id:" << product.id
         << ", quantity:" << product.quantity
         << ", unit price:" << product.unit_price
         << "}" << "\n";
}

void print_products(const unordered_map<int, Product>& products) {
    for (auto const& pair: products) {
        auto product = pair.second;
        print_product(product);
    }
}
