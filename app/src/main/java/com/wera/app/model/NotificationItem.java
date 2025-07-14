package com.wera.app.model;

public class NotificationItem {
    public enum Type { ALERT, ERROR, DEVICE }

    private final Type type;
    private final String message;

    public NotificationItem(Type type, String message) {
        this.type = type;
        this.message = message;
    }

    public Type getType() {
        return type;
    }

    public String getMessage() {
        return message;
    }

    public String getTypeText() {
        switch (type) {
            case ALERT: return "ALERT";
            case ERROR: return "ERROR";
            case DEVICE: return "DEVICE";
            default: return "";
        }
    }
}
