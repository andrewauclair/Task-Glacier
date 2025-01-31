package panels;

import io.github.andrewauclair.moderndocking.Dockable;

import javax.swing.*;

public class WeeklyReportPanel extends JPanel implements Dockable {
    public WeeklyReportPanel() {

    }

    @Override
    public String getPersistentID() {
        return "weekly-report";
    }

    @Override
    public String getTabText() {
        return "Weekly Report";
    }
}
