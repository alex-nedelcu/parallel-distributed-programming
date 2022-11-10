#include <iostream>
#include <vector>
#include <thread>

using std::cout;
using std::endl;
using std::vector;
using std::thread;
using std::mutex;

#define THREAD_COUNT 4
#define position vector<int>

vector<vector<int>> first_matrix{{1, 2, 3},
                                 {4, 5, 6},
                                 {7, 8, 9}};

vector<vector<int>> second_matrix{{1, 2, 3},
                                  {0, 3, 4},
                                  {0, 4, 5}};

unsigned long RESULT_MATRIX_ROW_COUNT = first_matrix.size();
unsigned long RESULT_MATRIX_COLUMN_COUNT = second_matrix.at(0).size();
unsigned long RESULT_MATRIX_ELEMENT_COUNT = RESULT_MATRIX_ROW_COUNT * RESULT_MATRIX_COLUMN_COUNT;

mutex assignments_mutex;

vector<bool> assigned(RESULT_MATRIX_ELEMENT_COUNT, false);

vector<int> get_column(const vector<vector<int>> &matrix, int column) {
    vector<int> target_column;

    for (int row = 0; row < matrix.size(); row += 1) {
        vector<int> current_row = matrix.at(row);
        target_column.push_back(current_row.at(column));
    }

    return target_column;
}

int compute_element_on_position(int row, int column) {
    vector<int> from_first_matrix = first_matrix.at(row);
    vector<int> from_second_matrix = get_column(second_matrix, column);

    int result_element = 0;

    for (int index = 0; index < from_first_matrix.size(); index++) {
        result_element += from_first_matrix.at(index) * from_second_matrix.at(index);
    }

    return result_element;
}

vector<position > find_positions_to_compute(int index) {
    vector<position > positions;

    for (int row = 0; row < RESULT_MATRIX_ROW_COUNT; row++) {
        for (int column = 0; column < RESULT_MATRIX_COLUMN_COUNT; column++) {
            int linear_position = row * RESULT_MATRIX_COLUMN_COUNT + column;

            assignments_mutex.lock();
            if (linear_position % THREAD_COUNT == index && !assigned.at(linear_position)) {
                assigned.at(linear_position) = true;

                position result_matrix_position{row, column};
                positions.push_back(result_matrix_position);
            }
            assignments_mutex.unlock();

        }
    }

    return positions;
}

void compute_partial_result_matrix(int index) {
    vector<position > positions_to_compute = find_positions_to_compute(index);

    assignments_mutex.lock();
    cout << "Thread " << index << endl;
    for (const auto &pos: positions_to_compute) {
        cout << pos.at(0) << " , " << pos.at(1) << endl;
    }
    cout << "---" << endl;

    assignments_mutex.unlock();

}

int main() {
    thread matrix_multiplication_worker[THREAD_COUNT];

    for (int index = 0; index < THREAD_COUNT; index++) {
        matrix_multiplication_worker[index] = thread(compute_partial_result_matrix, index);
    }

    for (int index = 0; index < THREAD_COUNT; index++) {
        matrix_multiplication_worker[index].join();
    }

    return 0;
}