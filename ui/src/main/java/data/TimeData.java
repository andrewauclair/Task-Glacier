package data;

import packets.TimeCategoriesMessage;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;

public class TimeData {
    public static class TimeCode {
        public int id;
        public String name;
    }
    public static class TimeCategory {
        public int id;
        public String name = "";
        public String label = "";

        public List<TimeCode> timeCodes = new ArrayList<>();
    }

    public static class TimeEntry {
        public int category;
        public int code;
    }

    List<TimeCategory> timeCategories = new ArrayList<>();

    public void processPacket(TimeCategoriesMessage data) {
        timeCategories = new ArrayList<>(data.getTimeCategories());
    }

    public List<TimeCategory> getTimeCategories() {
        return timeCategories;
    }

    public TimeCategory timeCategoryForTimeCode(int timeCodeID) {
        for (TimeCategory timeCategory : timeCategories) {
            if (timeCategory.timeCodes.stream().anyMatch(code -> code.id == timeCodeID)) {
                return timeCategory;
            }
        }
        return null;
    }

    public TimeCategory findTimeCategory(int timeCategoryID) {
        return timeCategories.stream()
                .filter(category -> category.id == timeCategoryID)
                .findFirst().orElse(null);
    }

    public TimeCode findTimeCode(int timeCodeID) {
        for (TimeCategory timeCategory : timeCategories) {
            Optional<TimeCode> result = timeCategory.timeCodes.stream()
                    .filter(code -> code.id == timeCodeID)
                    .findFirst();

            if (result.isPresent()) {
                return result.get();
            }
        }
        return null;
    }
}
