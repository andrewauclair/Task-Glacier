package panels;

import io.github.andrewauclair.moderndocking.Dockable;
import io.github.andrewauclair.moderndocking.app.Docking;
import packets.DailyReportMessage;

import javax.swing.*;

public class DailyReportPanel extends JPanel implements Dockable {
    JLabel date = new JLabel();

    public DailyReportPanel() {
        Docking.registerDockable(this);

        add(date);
    }

    public void update(DailyReportMessage message) {
        DailyReportMessage.DailyReport report = message.getReport();
        date.setText(String.format("%d/%d/%d", report.month, report.day, report.year));
    }

    @Override
    public String getPersistentID() {
        return "daily-report";
    }

    @Override
    public String getTabText() {
        return "Daily Report";
    }
}
