import java.security.Timestamp;
import java.time.LocalDateTime;
import java.util.Deque;
import java.util.LinkedList;
import java.util.Map;

interface RateLimiter{
    boolean isAllowed(String userId);
}

class TokenBucketRateLimitingAlgorithm implements RateLimiter{
    Map<String, TokenBucket> tokenBuckets;

    @Override
    public boolean isAllowed(String userId) {
        TokenBucket userTB = tokenBuckets.get(userId);

        if(userTB.checkIfAllowed()){
            return true;
        }else{
            return false;
        }
    }
}

class TokenBucket{
    int capacity;
    int tokens;
    long lastRefillTime;
    float refillRate;    // 1 token /3600

    public TokenBucket(){

    }
    private void refill(){
        long currentTime = System.currentTimeMillis();
        long diff = (currentTime - lastRefillTime)/1000;
        int eligibleToken =  (int)(diff * refillRate);
        tokens = Math.min(capacity, tokens + eligibleToken);

        lastRefillTime = System.currentTimeMillis();
    }
    boolean checkIfAllowed(){
        refill();
        if(tokens >= 1){
            tokens--;
            return true;
        }else{
            return false;
        }
    }
}

// Q : Removing from front, inserting at the end....
class SlidingWindowStrategy implements RateLimiter{
    Map<String, SlidingWindow> tokenBuckets;
    @Override
    public boolean isAllowed(String userId) {
        
    }
}

//Per User Sliding Windows
class SlidingWindow{
    Deque<Long> queue = new LinkedList<>();
    long windowSize = 60*1000; // 60 seconds 
    int maximum_request_allowed_in_window = 5;
    

    boolean isAllowed(){
        Long currentTime = System.currentTimeMillis()/1000;
        if(queue.size() < maximum_request_allowed_in_window)    //10
        {
            queue.offer(currentTime);
        }else{
                    //10 AM        2 Hr 
            while(currentTime - windowSize > queue.peek()){
                queue.remove();
            }
            if(queue.size() < maximum_request_allowed_in_window)
                queue.offer(currentTime);
            else{
                return false;
            }
        }
        return true;
    }
}

public class RateLimiterDemo {
    
}
