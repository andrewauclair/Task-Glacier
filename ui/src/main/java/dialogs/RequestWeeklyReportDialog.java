package dialogs;

import org.jdesktop.swingx.JXDatePicker;
import packets.RequestID;
import packets.RequestWeeklyReport;
import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;
import java.awt.*;
import java.time.LocalDate;
import java.time.ZoneId;
import java.util.Calendar;

public class RequestWeeklyReportDialog extends JDialog {
    public RequestWeeklyReportDialog(MainFrame mainFrame) {
        setLayout(new FlowLayout());
        setModal(true);

        DialogEscape.addEscapeHandler(this);

        JXDatePicker picker = new JXDatePicker();
        picker.setDate(Calendar.getInstance().getTime());
//        picker.setFormats(new SimpleDateFormat("dd.MM.yyyy"));

        add(picker);

        JButton send = new JButton("Send");
        add(send);

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        send.addActionListener(e -> {
            LocalDate localDate = picker.getDate().toInstant().atZone(ZoneId.systemDefault()).toLocalDate();
            int year = localDate.getYear();
            int month = localDate.getMonthValue();
            int day = localDate.getDayOfMonth();

            RequestWeeklyReport request = new RequestWeeklyReport();
            request.requestID = RequestID.nextRequestID();
            request.month = month;
            request.day = day;
            request.year = year;

            mainFrame.getConnection().sendPacket(request);

            RequestWeeklyReportDialog.this.dispose();
        });
    }
}
