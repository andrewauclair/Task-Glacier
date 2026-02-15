package dialogs;

import config.Sessions;
import data.TimeData;

import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.List;
import java.util.Optional;

public class UpdateSessionTimeCode extends JDialog {
    private int currentSession = 0;

    public UpdateSessionTimeCode(Window parent, List<Sessions.SessionRow> sessions, TimeData.TimeCategory category, TimeData.TimeCode newCode) {
        super(parent);

        setTitle("Update Session Time Code");

        setModalityType(ModalityType.APPLICATION_MODAL);

        JButton apply = new JButton("Apply");
        JButton skip = new JButton("Skip");

        // session #X
        // start time            stop time
        //
        // Current Time Entry
        // category - code
        //
        // New Time Entry
        // category - code
        //
        // apply skip

        GridBagConstraints gbc = new GridBagConstraints();
        gbc.gridx = 0;
        gbc.gridy = 0;
        gbc.weightx = 0;
        gbc.weighty = 0;
        gbc.fill = GridBagConstraints.NONE;

        setLayout(new GridBagLayout());

        add(new JLabel("Update Time Code for Time Category: " + category.name), gbc);
        gbc.gridy++;

        JLabel sessionNumber = new JLabel("Session #0");
        add(sessionNumber, gbc);
        gbc.gridy++;

        JLabel currentTimeCode = new JLabel("Current Time Code: ");
        add(currentTimeCode, gbc);
        gbc.gridy++;

        JLabel newTimeCode = new JLabel("New Time Code: " + newCode.name);
        add(newTimeCode, gbc);
        gbc.gridy++;

        JPanel buttons = new JPanel(new FlowLayout());
        buttons.add(apply);
        buttons.add(skip);

        add(buttons, gbc);
        gbc.gridy++;

        ActionListener listener = e -> {
            Sessions.SessionRow sessionRow = sessions.get(currentSession);
            Optional<TimeData.TimeEntry> entry = sessionRow.timeEntry.stream()
                    .filter(timeEntry -> timeEntry.category == category)
                    .findFirst();

            // TODO this probably causes issues, but we should never run into an empty entry
            if (!entry.isEmpty() && e != null && e.getSource() == apply) {
                sessionRow.modified = true;
                sessionRow.timeEntry.remove(entry.get());
                sessionRow.timeEntry.add(new TimeData.TimeEntry(entry.get().category, newCode));
            }

            currentSession++;

            if (currentSession >= sessions.size()) {
                UpdateSessionTimeCode.this.dispose();
                return;
            }

            sessionRow = sessions.get(currentSession);
            entry = sessionRow.timeEntry.stream()
                    .filter(timeEntry -> timeEntry.category == category)
                    .findFirst();

            while (entry.isEmpty() && currentSession < sessions.size()) {
                currentSession++;

                if (currentSession >= sessions.size()) {
                    UpdateSessionTimeCode.this.dispose();
                    return;
                }

                sessionRow = sessions.get(currentSession);
                entry = sessionRow.timeEntry.stream()
                        .filter(timeEntry -> timeEntry.category == category)
                        .findFirst();
            }

            if (currentSession >= sessions.size()) {
                UpdateSessionTimeCode.this.dispose();
                return;
            }

            sessionNumber.setText("Session #" + (currentSession + 1));
            currentTimeCode.setText("Current Time Code: " + entry.get().code.name);
        };
        apply.addActionListener(listener);
        skip.addActionListener(listener);

        Sessions.SessionRow sessionRow = sessions.get(currentSession);
        Optional<TimeData.TimeEntry> entry = sessionRow.timeEntry.stream()
                .filter(timeEntry -> timeEntry.category == category)
                .findFirst();

        sessionNumber.setText("Session #1");
        currentTimeCode.setText("Current Time Code: " + entry.get().code.name);

        pack();

        // center on the main frame
        setLocationRelativeTo(parent);
    }
}
