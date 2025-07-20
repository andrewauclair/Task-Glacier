package data;

import packets.TimeCategoriesMessage;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

public class TimeData {
    public static class TimeCode {
        public int id;
        public String name;

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) return false;
            TimeCode timeCode = (TimeCode) o;
            return id == timeCode.id;
        }

        @Override
        public int hashCode() {
            return Objects.hashCode(id);
        }
    }

    public static class TimeCategory {
        public int id;
        public String name = "";
        public String label = "";

        public List<TimeCode> timeCodes = new ArrayList<>();

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) return false;
            TimeCategory that = (TimeCategory) o;
            return id == that.id;
        }

        @Override
        public int hashCode() {
            return Objects.hashCode(id);
        }
    }

    public static class TimeEntry {
        public TimeCategory category;
        public TimeCode code;

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) return false;
            TimeEntry timeEntry = (TimeEntry) o;
            return category.equals(timeEntry.category) && code.equals(timeEntry.code);
        }

        @Override
        public int hashCode() {
            return Objects.hash(category, code);
        }
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
