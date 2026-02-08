package data;

import packets.TimeEntryData;
import packets.TimeEntryModify;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Optional;

public class TimeData {
    List<TimeCategory> timeCategories = new ArrayList<>();

    public void processPacket(TimeEntryData data) {
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

    public static class TimeCode {
        public int id;
        public String name;
        public boolean archived;

        public TimeCode() {
        }

        public TimeCode(int id, String name) {
            this.id = id;
            this.name = name;
        }

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) {
                return false;
            }
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

        public List<TimeCode> timeCodes = new ArrayList<>();

        public TimeCategory() {
        }

        public TimeCategory(String name, int id) {
            this.name = name;
            this.id = id;
        }

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) {
                return false;
            }
            TimeCategory that = (TimeCategory) o;
            return id == that.id;
        }

        @Override
        public int hashCode() {
            return Objects.hashCode(id);
        }
    }

    public static class TimeEntry {
        public final TimeCategory category;
        public final TimeCode code;

        public TimeEntry(TimeCategory category, TimeCode code) {
            this.category = category;
            this.code = code;
        }

        @Override
        public boolean equals(Object o) {
            if (o == null || getClass() != o.getClass()) {
                return false;
            }
            TimeEntry timeEntry = (TimeEntry) o;
            return category.equals(timeEntry.category) && code.equals(timeEntry.code);
        }

        @Override
        public int hashCode() {
            return Objects.hash(category, code);
        }
    }
}
