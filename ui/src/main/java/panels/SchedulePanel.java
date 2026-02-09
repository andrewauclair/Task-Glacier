package panels;

import io.github.andrewauclair.moderndocking.Dockable;

import javax.swing.*;

public class SchedulePanel extends JPanel implements Dockable {
    public SchedulePanel() {
        // maybe this is also a tree like the tasks? a little different though
        // we would only show the parent task, or whatever is listed in the priority dialog

        // we'll list tasks out as far as they can go using their estimates

        // not sure what to do if a task is over the estimate, or when it has no estimate

        // the main goal of this schedule is to relay how many more days a project will take

        // maybe it can get more advanced and factor in how much time I'm actually spending.
        // like compare the suggested schedule for the day to what actually happens and adjust the following
        // days appropriately based on that
    }

    @Override
    public String getPersistentID() {
        return "schedule";
    }

    @Override
    public String getTabText() {
        return "Schedule";
    }
}
