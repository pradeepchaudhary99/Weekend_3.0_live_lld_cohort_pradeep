/*
FunctionalRequirements:

Users should be able to send Notification
System should support multiple types of channels
    --> Email
    --> SMS
    --> Push
    --> Whatsapp

Notification System should care about user preference
Notification sending should be asynchronous
Easy to add new notification types/channels
Notifications should be resilient to transient channel failures (retry)
Notifications should be rate-limited per recipient per channel


Non-Functional Requirements:
Thread-safe: preferences and channel state are shared across worker threads
Async: sending never blocks the caller
Extensible: new channels/behaviors pluggable via Factory + Decorator


Entities:

User --> Client ---> main --> class Clients
            // SendNotification

Notification
NotificationService
NotificationChannel
UserPreferencesRepository
NotificationChannelFactory

Applying Decorator Design Pattern:
    RateLimiterDecorator, RetryDecorator wrap any NotificationChannel to add
    cross-cutting behavior without touching the concrete channel classes.
*/

import java.util.EnumMap;
import java.util.EnumSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

class Notification {
    private final String recipient;
    private final String title;
    private final String message;

    public Notification(String recipient, String title, String message) {
        this.recipient = recipient;
        this.title = title;
        this.message = message;
    }

    public String getRecipient() {
        return recipient;
    }

    public String getTitle() {
        return title;
    }

    public String getMessage() {
        return message;
    }
}

class NotificationDeliveryException extends RuntimeException {
    public NotificationDeliveryException(String message) {
        super(message);
    }
}

interface NotificationChannel {
    void sendNotification(Notification notification);
}

// ---------- Concrete channels ----------

class SMSNotification implements NotificationChannel {
    @Override
    public void sendNotification(Notification notification) {
        System.out.println("[SMS] to " + notification.getRecipient() + ": " + notification.getTitle());
    }
}

class PushNotification implements NotificationChannel {
    @Override
    public void sendNotification(Notification notification) {
        System.out.println("[PUSH] to " + notification.getRecipient() + ": " + notification.getTitle());
    }
}

class WhatsappNotification implements NotificationChannel {
    @Override
    public void sendNotification(Notification notification) {
        System.out.println("[WHATSAPP] to " + notification.getRecipient() + ": " + notification.getTitle());
    }
}

class EmailNotification implements NotificationChannel {
    @Override
    public void sendNotification(Notification notification) {
        System.out.println("[EMAIL] to " + notification.getRecipient() + ": " + notification.getTitle());
    }
}

// ---------- Decorators (Decorator pattern) ----------

abstract class NotificationDecorator implements NotificationChannel {
    protected final NotificationChannel notificationChannel;

    public NotificationDecorator(NotificationChannel notificationChannel) {
        this.notificationChannel = notificationChannel;
    }
}

// Simple fixed-window counter per recipient: at most `limit` sends per `windowMillis`.
// Thread-safe: each recipient's counter is a distinct AtomicInteger/window pair,
// created atomically via computeIfAbsent.
class RateLimiterDecorator extends NotificationDecorator {

    private static class Window {
        final AtomicInteger count = new AtomicInteger();
        volatile long windowStart = System.currentTimeMillis();
    }

    private final int limit;
    private final long windowMillis;
    private final Map<String, Window> windows = new ConcurrentHashMap<>();

    public RateLimiterDecorator(NotificationChannel notificationChannel, int limit, long windowMillis) {
        super(notificationChannel);
        this.limit = limit;
        this.windowMillis = windowMillis;
    }

    @Override
    public void sendNotification(Notification notification) {
        Window window = windows.computeIfAbsent(notification.getRecipient(), r -> new Window());
        synchronized (window) {
            long now = System.currentTimeMillis();
            if (now - window.windowStart > windowMillis) {
                window.windowStart = now;
                window.count.set(0);
            }
            if (window.count.incrementAndGet() > limit) {
                System.out.println("[RateLimited] dropping notification to "
                        + notification.getRecipient());
                return;
            }
        }
        notificationChannel.sendNotification(notification);
    }
}

// Retries the wrapped channel with a short backoff on transient failures.
class RetryDecorator extends NotificationDecorator {

    private final int maxAttempts;
    private final long backoffMillis;

    public RetryDecorator(NotificationChannel notificationChannel, int maxAttempts, long backoffMillis) {
        super(notificationChannel);
        this.maxAttempts = maxAttempts;
        this.backoffMillis = backoffMillis;
    }

    @Override
    public void sendNotification(Notification notification) {
        RuntimeException lastFailure = null;
        for (int attempt = 1; attempt <= maxAttempts; attempt++) {
            try {
                notificationChannel.sendNotification(notification);
                return;
            } catch (NotificationDeliveryException e) {
                lastFailure = e;
                System.out.println("Attempt " + attempt + " failed for " + notification.getRecipient()
                        + ": " + e.getMessage());
                try {
                    Thread.sleep(backoffMillis);
                } catch (InterruptedException ie) {
                    Thread.currentThread().interrupt();
                    return;
                }
            }
        }
        System.out.println("Giving up on notification to " + notification.getRecipient()
                + " after " + maxAttempts + " attempts");
        if (lastFailure != null) {
            throw lastFailure;
        }
    }
}

// ---------- Factory (Factory pattern) ----------

enum NotificationType {
    SMS, PUSH, EMAIL, WHATSAPP
}

class NotificationChannelFactory {

    private final Map<NotificationType, NotificationChannel> registry = new EnumMap<>(NotificationType.class);

    public NotificationChannelFactory() {
        registry.put(NotificationType.EMAIL, decorate(new EmailNotification()));
        registry.put(NotificationType.SMS, decorate(new SMSNotification()));
        registry.put(NotificationType.PUSH, decorate(new PushNotification()));
        registry.put(NotificationType.WHATSAPP, decorate(new WhatsappNotification()));
    }

    // Every channel gets retry-on-failure and per-recipient rate limiting for free.
    private NotificationChannel decorate(NotificationChannel base) {
        return new RetryDecorator(new RateLimiterDecorator(base, 3, 1000), 2, 50);
    }

    public NotificationChannel getInstance(NotificationType type) {
        NotificationChannel channel = registry.get(type);
        if (channel == null) {
            throw new IllegalArgumentException("No channel registered for type " + type);
        }
        return channel;
    }
}

// ---------- User preferences ----------

class UserPreferencesRepository {
    private final Map<String, Set<NotificationType>> userPreferences = new ConcurrentHashMap<>();

    public void setUserPreference(String userId, Set<NotificationType> types) {
        userPreferences.put(userId, EnumSet.copyOf(types));
    }

    public Set<NotificationType> getUserPreference(String userId) {
        return userPreferences.getOrDefault(userId, EnumSet.of(NotificationType.EMAIL));
    }
}

// Orchestrator / Facade
class NotificationService {
    private final ExecutorService executor;
    private final NotificationChannelFactory factory;
    private final UserPreferencesRepository userPreferencesRepository;

    public NotificationService(UserPreferencesRepository userPreferencesRepository) {
        this.executor = Executors.newFixedThreadPool(4);
        this.factory = new NotificationChannelFactory();
        this.userPreferencesRepository = userPreferencesRepository;
    }

    public void send(Notification notification) {
        Set<NotificationType> preferredTypes = userPreferencesRepository.getUserPreference(notification.getRecipient());
        for (NotificationType type : preferredTypes) {
            NotificationChannel channel = factory.getInstance(type);
            // Async: fan out to every preferred channel without blocking the caller.
            executor.submit(() -> channel.sendNotification(notification));
        }
    }

    public void shutdown() throws InterruptedException {
        executor.shutdown();
        executor.awaitTermination(5, TimeUnit.SECONDS);
    }
}

class NotificationClient {
    void sendNotification(NotificationService notificationService, Notification notification) {
        notificationService.send(notification);
    }
}

public class NotificationSystem_demo {

    public static void main(String[] args) throws InterruptedException {
        UserPreferencesRepository preferences = new UserPreferencesRepository();
        preferences.setUserPreference("alice@example.com", EnumSet.of(NotificationType.EMAIL, NotificationType.SMS));
        preferences.setUserPreference("bob@example.com", EnumSet.of(NotificationType.PUSH, NotificationType.WHATSAPP));

        NotificationService service = new NotificationService(preferences);
        NotificationClient client = new NotificationClient();

        CountDownLatch latch = new CountDownLatch(6);
        ExecutorService callers = Executors.newFixedThreadPool(2);
        Runnable done = latch::countDown;

        for (int i = 0; i < 3; i++) {
            final int idx = i;
            callers.submit(() -> {
                client.sendNotification(service,
                        new Notification("alice@example.com", "Order Update " + idx, "Your order shipped"));
                done.run();
            });
            callers.submit(() -> {
                client.sendNotification(service,
                        new Notification("bob@example.com", "Promo " + idx, "50% off today"));
                done.run();
            });
        }

        callers.shutdown();
        callers.awaitTermination(5, TimeUnit.SECONDS);
        latch.await();

        service.shutdown();
        System.out.println("All notifications dispatched.");
    }
}
