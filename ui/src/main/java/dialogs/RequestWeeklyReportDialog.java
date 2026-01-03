package dialogs;

import org.jdesktop.swingx.JXDatePicker;
import packets.RequestID;
import packets.RequestWeeklyReport;
import raven.datetime.DatePicker;
import taskglacier.MainFrame;
import util.DialogEscape;

import javax.swing.*;
import java.awt.*;
import java.time.LocalDate;
import java.time.ZoneId;
import java.util.Calendar;

public class RequestWeeklyReportDialog extends JDialog {
    public RequestWeeklyReportDialog(MainFrame mainFrame) {
        super(mainFrame);

        setTitle("Request Weekly Report");

        setLayout(new GridBagLayout());

        setModalityType(ModalityType.APPLICATION_MODAL);

        DialogEscape.addEscapeHandler(this);

        DatePicker picker = new DatePicker();
        picker.setSelectedDate(LocalDate.now());

        picker.setDateSelectionMode(DatePicker.DateSelectionMode.BETWEEN_DATE_SELECTED);

        picker.addDateSelectionListener(dateSelectionEvent -> {
            picker.setSelectedDateRange(picker.getSelectedDate(), picker.getSelectedDate().plusDays(6));
        });

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.anchor = GridBagConstraints.CENTER;
        gbc.insets = new Insets(5, 5, 5, 5);
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0;
        gbc.weighty = 0;

        add(picker, gbc);
        gbc.gridy++;

        JButton send = new JButton("Send");

        add(send, gbc);
        gbc.gridy++;

        pack();

        // center on the main frame
        setLocationRelativeTo(mainFrame);

        send.addActionListener(e -> {
            LocalDate localDate = picker.getSelectedDate();

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
