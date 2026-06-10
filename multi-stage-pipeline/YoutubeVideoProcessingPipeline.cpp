/*
Design a YouTube Video Processing Pipeline that processes uploaded videos
asynchronously. The pipeline should: Accept video upload requests from multiple
users. Store uploaded videos in a thread-safe queue. Process videos
asynchronously using a background worker. Perform multiple processing stages for
each video: Transcode Generate thumbnails Publish Shut down gracefully, ensuring
all queued videos are processed before exiting.

Architecture:
             User Uploads
          (Multiple Producers)
                    │
                    ▼
        +----------------------+
        | Thread-safe Queue    |
        +----------------------+
                    │
          Condition Variable
                    │
                    ▼
        +----------------------+
        | Background Worker    |
        +----------------------+
                    │
        ┌───────────┼────────────┐
        ▼           ▼            ▼
   Transcode   Thumbnail    Metadata
                    │
                    ▼
              Publish Video

Complexity: TC = SC = O(number of queued videos)

Production Enhancements:
Multiple worker threads for parallel video processing.
Separate queues for different stages (transcoding, thumbnail generation,
publishing). Prioritize premium users or live streams. Store videos in object
storage (e.g., S3/GCS). Use a message broker (Kafka/RabbitMQ) between stages.
Retry failed processing and maintain a dead-letter queue.
Track progress and expose processing status.
Autoscale workers based on queue length.
*/
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

using namespace std;

struct Video {
  int id;
  string title;
};

class VideoPipeline {

  mutex mtx, cout_mtx;
  condition_variable cv;

  bool stop = false;
  thread worker;

  queue<Video> upload_queue;

  void process_video() {

    while (true) {

      Video video;
      {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [&]() { return stop || !upload_queue.empty(); });

        if (stop && upload_queue.empty())
          break;

        video = upload_queue.front();
        upload_queue.pop();
      }

      transcode(video);
      generate_thumbnail(video);
      publish(video);
    }
  }

  void transcode(const Video &video) {

    {
      lock_guard<mutex> lock(cout_mtx);
      cout << "Transcoding : " << video.title << endl;
    }
  }

  void generate_thumbnail(const Video &video) {

    {
      lock_guard<mutex> lock(mtx);
      cout << "Generating Thumbnail : " << video.title << endl;
    }
  }

  void publish(const Video &video) {

    {
      lock_guard<mutex> lock(mtx);
      cout << "Publishing : " << video.title << endl;
    }
  }

public:
  VideoPipeline() { worker = thread(&VideoPipeline::process_video, this); }

  void upload(const Video &video) {
    {
      lock_guard<mutex> lock(mtx);
      upload_queue.push(video);
    }
    cv.notify_one();
  }

  ~VideoPipeline() {
    {
      lock_guard<mutex> lock(mtx);
      stop = true;
    }
    cv.notify_all();

    if (worker.joinable())
      worker.join();
  }
};

int main() {

  VideoPipeline pipeline;

  thread user1([&]() {
    for (int i = 1; i <= 3; i++)
      pipeline.upload({i, "Travel_vlog_" + to_string(i)});
  });

  thread user2([&]() {
    for (int i = 4; i <= 6; i++)
      pipeline.upload({i, "Cooking_video_" + to_string(i)});
  });

  user1.join();
  user2.join();

  return 0;
}
/*
Output:
Transcoding : Travel_vlog_1
Generating Thumbnail : Travel_vlog_1
Publishing : Travel_vlog_1
Transcoding : Cooking_video_4
Generating Thumbnail : Cooking_video_4
Publishing : Cooking_video_4
Transcoding : Cooking_video_5
Generating Thumbnail : Cooking_video_5
Publishing : Cooking_video_5
Transcoding : Cooking_video_6
Generating Thumbnail : Cooking_video_6
Publishing : Cooking_video_6
Transcoding : Travel_vlog_2
Generating Thumbnail : Travel_vlog_2
Publishing : Travel_vlog_2
Transcoding : Travel_vlog_3
Generating Thumbnail : Travel_vlog_3
Publishing : Travel_vlog_3
*/
