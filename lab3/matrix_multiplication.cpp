#include <iostream>
#include <functional>
#include <queue>
#include <thread>

using std::condition_variable;
using std::queue;
using std::vector;
using std::cout;
using std::endl;
using std::thread;
using std::function;
using std::mutex;
using std::unique_lock;

#define THREAD_COUNT 4
#define position vector<int>
#define INITIAL_VALUE (-99999)

vector<vector<int>> first_matrix{{6,  1, 6},
                                 {3,  1, 6},
                                 {43, 2, 3}};

vector<vector<int>> second_matrix{{1, 4},
                                  {2, 6},
                                  {3, 12}};

vector<vector<int>> result_matrix;

unsigned long RESULT_MATRIX_ROW_COUNT = first_matrix.size();
unsigned long RESULT_MATRIX_COLUMN_COUNT = second_matrix[0].size();
unsigned long RESULT_MATRIX_ELEMENT_COUNT = RESULT_MATRIX_ROW_COUNT * RESULT_MATRIX_COLUMN_COUNT;

class ThreadPool {

public:

    void start();

    void queue_job(const function<void()> &job);

    void stop();

private:

    void thread_loop();

    bool should_terminate = false;

    mutex queue_mutex;

    condition_variable mutex_condition;

    vector<thread> threads;

    queue<function<void()>> jobs;
};

void ThreadPool::start() {
    const uint32_t thread_count = thread::hardware_concurrency(); // number of threads supported by system
    threads.resize(thread_count);

    for (int i = 0; i < thread_count; i++) {
        threads[i] = thread(
                [this] {
                    this->thread_loop();
                }
        );
    }
}

void ThreadPool::thread_loop() {
    while (true) {

        function<void()> job;

        {
            unique_lock<mutex> lock(queue_mutex);

            mutex_condition.wait(lock, [this] {
                return !jobs.empty() || should_terminate;
            });

            if (should_terminate) {
                return;
            }

            job = jobs.front();
            jobs.pop();
        }

        job();
    }
}

void ThreadPool::queue_job(const function<void()> &job) {
    {
        unique_lock<mutex> lock(queue_mutex);
        jobs.push(job);
    }

    mutex_condition.notify_one();
}

void ThreadPool::stop() {
    {
        unique_lock<mutex> lock(queue_mutex);
        should_terminate = true;
    }

    mutex_condition.notify_all();

    for (thread &active_thread: threads) {
        active_thread.join();
    }

    threads.clear();
}

vector<int> get_column(const vector<vector<int>> &matrix, int column) {
    vector<int> target_column;

    for (int row = 0; row < matrix.size(); row += 1) {
        vector<int> current_row = matrix[row];
        target_column.push_back(current_row[column]);
    }

    return target_column;
}

int compute_element_on_position(int row, int column) {
    vector<int> from_first_matrix = first_matrix[row];
    vector<int> from_second_matrix = get_column(second_matrix, column);

    int result_element = 0;

    for (int index = 0; index < from_first_matrix.size(); index++) {
        result_element += from_first_matrix[index] * from_second_matrix[index];
    }

    return result_element;
}

vector<position > find_positions_to_compute(int index) {
    vector<position > positions;

    for (int row = 0; row < RESULT_MATRIX_ROW_COUNT; row++) {
        for (int column = 0; column < RESULT_MATRIX_COLUMN_COUNT; column++) {
            int linear_position = row * RESULT_MATRIX_COLUMN_COUNT + column;

            if (linear_position % THREAD_COUNT == index) {
                position result_matrix_position{row, column};
                positions.push_back(result_matrix_position);
            }

        }
    }

    return positions;
}

void compute_partial_result_matrix(int index) {
    vector<position > positions_to_compute = find_positions_to_compute(index);

    for (const auto &row_column_pair: positions_to_compute) {
        int row = row_column_pair[0];
        int column = row_column_pair[1];

        result_matrix[row][column] = compute_element_on_position(row, column);
    }
}

void initialize_result_matrix() {
    for (int row = 0; row < RESULT_MATRIX_ROW_COUNT; row++) {
        vector<int> column(RESULT_MATRIX_COLUMN_COUNT, INITIAL_VALUE);
        result_matrix.push_back(column);
    }
}

void print_matrix(const vector<vector<int>> matrix) {
    for (int row = 0; row < matrix.size(); row++) {
        for (int column = 0; column < matrix[row].size(); column++) {
            cout << matrix[row][column] << " ";
        }

        cout << endl;
    }
}

void compute_with_thread_pool_service() {
    ThreadPool service{};

    for (int index = 0; index < THREAD_COUNT; index++) {
        service.queue_job(
                [index] {
                    compute_partial_result_matrix(index);
                }
        );
    }

    service.start();
    service.stop();
}

void compute_with_separate_tasks() {
    thread matrix_multiplication_worker[THREAD_COUNT];

    for (int index = 0; index < THREAD_COUNT; index++) {
        matrix_multiplication_worker[index] = thread(compute_partial_result_matrix, index);
    }

    for (int index = 0; index < THREAD_COUNT; index++) {
        matrix_multiplication_worker[index].join();
    }
}

int main() {
    initialize_result_matrix();

//     v1
//    compute_with_separate_tasks();

//     v2
    compute_with_thread_pool_service();

    print_matrix(result_matrix);

    return 0;
}