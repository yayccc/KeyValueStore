#pragma once


#include<iostream>
#include<thread>
#include<vector>
#include<queue>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<atomic>
#include<future>



class ThreadPool{
public:
    ThreadPool(int num_threads){
        for(int i=0;i<num_threads;i++){
            workers_.push_back(std::thread([this]{
                while(true){
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mutex_);
                        condition_.wait(lock, [this]{return !tasks_.empty() || stop_;});
                        if(stop_ && tasks_.empty()){
                            return;
                        }
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
            }));
        }
    }

    template<class F, class... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>{
        using return_type = typename std::result_of<F(Args...)>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if(stop_){
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task](){(*task)();});
        }
        condition_.notify_one();
        return res;
    }

    ~ThreadPool(){
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for(std::thread &t: workers_){
            t.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stop_ = false;

};


