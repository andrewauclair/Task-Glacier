package panels;

import io.github.andrewauclair.moderndocking.Dockable;

import javax.swing.*;

public class WeeklyReport extends JPanel implements Dockable {

    @Override
    public String getPersistentID() {
        return "weekly-report";
    }

    @Override
    public String getTabText() {
        return "Weekly Report";
    }
}
